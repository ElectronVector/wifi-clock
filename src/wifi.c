#include "wifi.h"

#include <stdio.h>
#include <string.h>

#include "zephyr/net/net_if.h"
#include "zephyr/net/net_ip.h"
#include "zephyr/net/dhcpv4.h"
#include "zephyr/net/net_mgmt.h"
#include "zephyr/net/wifi_mgmt.h"
#include "zephyr/sys/atomic.h"
#include "zephyr/kernel.h"
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(wifi, CONFIG_LOG_DEFAULT_LEVEL);

#define WIFI_IFACE_NAME_MAX 32

ZBUS_CHAN_DEFINE(wifi_event_chan, struct wifi_event_msg, NULL, NULL, ZBUS_OBSERVERS(network_wifi_sub),
		 ZBUS_MSG_INIT(.events = 0));

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback wifi_disconnect_cb;
static atomic_t wifi_connected = ATOMIC_INIT(0);

static void publish_wifi_event(uint32_t events)
{
	const struct wifi_event_msg msg = {
		.events = events,
	};
	int ret = zbus_chan_pub(&wifi_event_chan, &msg, K_NO_WAIT);

	if (ret) {
		LOG_ERR("Failed to publish Wi-Fi event 0x%08x: %d", events, ret);
	}
}

#ifdef CONFIG_WIFI
static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint64_t mgmt_event,
                                   struct net_if *iface)
{
	    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
	        struct wifi_status *status = (struct wifi_status *)cb->info;
	        if (status && status->status == 0) {
	            atomic_set(&wifi_connected, 1);
	            publish_wifi_event(WIFI_EVENT_CONNECTED);
	            LOG_INF("Wi-Fi connected event received");
	        } else {
	            LOG_ERR("Wi-Fi connect failed, status: %d", status ? status->status : -1);
	        }
	    } else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
	        if (atomic_cas(&wifi_connected, 1, 0)) {
	            publish_wifi_event(WIFI_EVENT_DISCONNECTED);
	            LOG_INF("Wi-Fi disconnected event received");
	        } else {
	            LOG_DBG("Ignoring disconnect event before initial connect");
	        }
	    }
}
#endif

static struct net_mgmt_event_callback dns_cb;

static void dns_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint64_t mgmt_event,
                                   struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_DNS_SERVER_ADD) {
        LOG_INF("DNS server added event received");
        publish_wifi_event(WIFI_EVENT_DNS_CONFIGURED);
    }
}

static struct net_if *get_default_iface(void)
{
	struct net_if *iface = net_if_get_default();
	char iface_name[WIFI_IFACE_NAME_MAX] = {0};

	if (net_if_get_name(iface, iface_name, sizeof(iface_name)) > 0) {
		LOG_INF("iface: %s", iface_name);
	} else {
		LOG_INF("iface: <unknown>");
	}
	LOG_INF("up: %d", net_if_is_up(iface));
	return iface;
}

static bool wifi_credentials_configured(void)
{
	if ((strcmp(CONFIG_WIFI_SSID, "NO_SSID_SET") == 0) ||
	    (strcmp(CONFIG_WIFI_PASSPHRASE, "NO_PASSPHRASE_SET") == 0)) {
		return false;
	}

	return (CONFIG_WIFI_SSID[0] != '\0');
}

#ifdef CONFIG_WIFI
static void register_wifi_callbacks(void)
{
	net_mgmt_init_event_callback(&wifi_cb, wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_CONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_cb);

	net_mgmt_init_event_callback(&wifi_disconnect_cb, wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_disconnect_cb);
}

static int request_wifi_connect(struct net_if *iface)
{
	struct wifi_connect_req_params wifi_params = {
		.ssid = CONFIG_WIFI_SSID,
		.ssid_length = strlen(CONFIG_WIFI_SSID),
		.psk = CONFIG_WIFI_PASSPHRASE,
		.psk_length = strlen(CONFIG_WIFI_PASSPHRASE),
		.security = WIFI_SECURITY_TYPE_PSK,
		.channel = WIFI_CHANNEL_ANY,
		.mfp = WIFI_MFP_OPTIONAL,
	};

	int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params,
			   sizeof(wifi_params));
	if (ret) {
		LOG_ERR("Wi-Fi connect request failed: %d", ret);
		return -1;
	}

	return 0;
}

static int wait_for_wifi_connected(void)
{
	uint32_t waited = 0;

	while (!atomic_get(&wifi_connected)) {
		k_msleep(100);
		waited += 100;
		if (waited >= CONFIG_WIFI_TIMEOUT_MS) {
			LOG_ERR("Wi-Fi connection timeout");
			return -1;
		}
	}

	LOG_INF("Wi-Fi connected (event)");
	return 0;
}
#endif

static void register_dns_callback(void)
{
	net_mgmt_init_event_callback(&dns_cb, dns_mgmt_event_handler,
				     NET_EVENT_DNS_SERVER_ADD);
	net_mgmt_add_event_callback(&dns_cb);
}

static int wait_for_dhcp_ipv4(struct net_if *iface)
{
	uint32_t dhcp_waited = 0;

	while (1) {
		struct in_addr *addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
		if (addr && !net_ipv4_is_addr_unspecified(addr)) {
			LOG_INF("DHCP IP address assigned: %s",
				net_addr_ntop(AF_INET, addr,
					      (char[NET_IPV4_ADDR_LEN]){0},
					      NET_IPV4_ADDR_LEN));
			return 0;
		}

		k_msleep(100);
		dhcp_waited += 100;
		if (dhcp_waited >= CONFIG_DHCP_TIMEOUT_MS) {
			LOG_ERR("DHCP IP address assignment timeout");
			return -1;
		}
	}
}

int wifi_connect()
{
	if (!wifi_credentials_configured()) {
		LOG_ERR("Wi-Fi credentials not configured (CONFIG_WIFI_SSID / CONFIG_WIFI_PASSPHRASE)");
		publish_wifi_event(WIFI_EVENT_DISCONNECTED);
		return -1;
	}

	LOG_INF("Connecting to network: %s", CONFIG_WIFI_SSID);
	struct net_if *iface = get_default_iface();

#ifdef CONFIG_WIFI
	atomic_set(&wifi_connected, 0);
	printf("Using SSID: %s\n", CONFIG_WIFI_SSID);

	register_wifi_callbacks();
#endif

	register_dns_callback();

#ifdef CONFIG_WIFI
	if (request_wifi_connect(iface) != 0) {
		return -1;
	}

	if (wait_for_wifi_connected() != 0) {
		return -1;
	}
#endif

	LOG_INF("Starting DHCPv4...");
	net_dhcpv4_start(iface);

	return wait_for_dhcp_ipv4(iface);
}

static void wifi_thread_fn(void *arg1, void *arg2, void *arg3)
{
	int ret;

	LOG_INF("Wi-Fi thread started");

	while (1) {
		ret = wifi_connect();
		if (ret == 0) {
			LOG_INF("Wi-Fi connected successfully");
			publish_wifi_event(WIFI_EVENT_IP_ACQUIRED);
			break;
		}

		LOG_ERR("Wi-Fi connection failed, retrying in 5 seconds...");
		k_sleep(K_SECONDS(5));
	}
}

K_THREAD_DEFINE(wifi_thread, 4096, wifi_thread_fn, NULL, NULL, NULL, 7, 0, 0);
