#include "wifi.h"

#include <stdio.h>

#include "zephyr/net/net_if.h"
#include "zephyr/net/dhcpv4.h"
#include "zephyr/net/net_mgmt.h"
#include "zephyr/net/wifi_mgmt.h"
#include "zephyr/sys/atomic.h"
#include "zephyr/kernel.h"

LOG_MODULE_REGISTER(wifi, CONFIG_LOG_DEFAULT_LEVEL);

#ifdef CONFIG_WIFI
K_EVENT_DEFINE(wifi_events);

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback wifi_disconnect_cb;
static atomic_t wifi_connected = ATOMIC_INIT(0);

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint32_t mgmt_event,
                                   struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        struct wifi_status *status = (struct wifi_status *)cb->info;
        if (status && status->status == 0) {
            atomic_set(&wifi_connected, 1);
            k_event_set(&wifi_events, WIFI_EVENT_CONNECTED);
            LOG_INF("Wi-Fi connected event received");
        } else {
            LOG_ERR("Wi-Fi connect failed, status: %d", status ? status->status : -1);
        }
    } else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        atomic_set(&wifi_connected, 0);
        k_event_set(&wifi_events, WIFI_EVENT_DISCONNECTED);
        LOG_INF("Wi-Fi disconnected event received");
    }
}
#else
struct k_event wifi_events;
static int wifi_events_init(void)
{
	k_event_init(&wifi_events);
	return 0;
}
SYS_INIT(wifi_events_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

static struct net_mgmt_event_callback dns_cb;

static void dns_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                   uint32_t mgmt_event,
                                   struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_DNS_SERVER_ADD) {
        LOG_INF("DNS server added event received");
        k_event_set(&wifi_events, WIFI_EVENT_DNS_CONFIGURED);
    }
}

int wifi_connect()
{
    LOG_INF("Connecting to network: %s", CONFIG_WIFI_SSID);
    struct net_if* iface = net_if_get_default();
    LOG_INF("iface: %s", iface->config.name);
    LOG_INF("up: %d", net_if_is_up(iface));

#ifdef CONFIG_WIFI
    printf("Using SSID: %s\n", CONFIG_WIFI_SSID);

    // Wi-Fi connection setup
    struct wifi_connect_req_params wifi_params = {
        .ssid = CONFIG_WIFI_SSID,
        .ssid_length = strlen(CONFIG_WIFI_SSID),
        .psk = CONFIG_WIFI_PASSPHRASE,
        .psk_length = strlen(CONFIG_WIFI_PASSPHRASE),
        .security = WIFI_SECURITY_TYPE_PSK,
        .channel = WIFI_CHANNEL_ANY,
        .mfp = WIFI_MFP_OPTIONAL,
    };

    // Register Wi-Fi event callbacks
    net_mgmt_init_event_callback(&wifi_cb, wifi_mgmt_event_handler, NET_EVENT_WIFI_CONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);

    net_mgmt_init_event_callback(&wifi_disconnect_cb, wifi_mgmt_event_handler, NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_disconnect_cb);
#endif

    net_mgmt_init_event_callback(&dns_cb, dns_mgmt_event_handler, NET_EVENT_DNS_SERVER_ADD);
    net_mgmt_add_event_callback(&dns_cb);

#ifdef CONFIG_WIFI
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params, sizeof(wifi_params));
    if (ret) {
        LOG_ERR("Wi-Fi connect request failed: %d", ret);
        return -1;
    }

    // Wait for the Wi-Fi connection event
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

#endif

    LOG_INF("Starting DHCPv4...");
    net_dhcpv4_start(iface);

    // Wait for DHCP to assign an IP address (timeout after 30 seconds)
    uint32_t dhcp_waited = 0;
    while (1) {
        struct net_if_addr_ipv4 *if_addr = NULL;
        int i;
        for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
            if_addr = &iface->config.ip.ipv4->unicast[i];
            if (if_addr->ipv4.addr_type == NET_ADDR_DHCP &&
                if_addr->ipv4.is_used &&
                !net_ipv4_is_addr_unspecified(&if_addr->ipv4.address.in_addr)) {
                LOG_INF("DHCP IP address assigned: %s", net_addr_ntop(AF_INET, &if_addr->ipv4.address.in_addr, (char[NET_IPV4_ADDR_LEN]){0}, NET_IPV4_ADDR_LEN));
                return 0;
            }
        }
        k_msleep(100);
        dhcp_waited += 100;
        if (dhcp_waited >= CONFIG_DHCP_TIMEOUT_MS) {
            LOG_ERR("DHCP IP address assignment timeout");
            return -1;
        }
    }

    return 0;
}

static void wifi_thread_fn(void *arg1, void *arg2, void *arg3)
{
    int ret;

    LOG_INF("Wi-Fi thread started");

    while (1) {
        ret = wifi_connect();
        if (ret == 0) {
            LOG_INF("Wi-Fi connected successfully");
            k_event_set(&wifi_events, WIFI_EVENT_IP_ACQUIRED);
            break;
        }

        LOG_ERR("Wi-Fi connection failed, retrying in 5 seconds...");
        k_sleep(K_SECONDS(5));
    }
}

K_THREAD_DEFINE(wifi_thread, 1024, wifi_thread_fn, NULL, NULL, NULL, 7, 0, 0);
