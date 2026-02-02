# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Print the value of the ZEPHYR_BASE environment variable during configure
message(STATUS "ZEPHYR_BASE environment variable: $ENV{ZEPHYR_BASE}")

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/private/wifi-network.conf")
  set(EXTRA_CONF_FILE "private/wifi-network.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi-clock)

target_include_directories(app PRIVATE src)
target_sources(app PRIVATE src/main.c src/network.c src/wifi.c src/led.c src/network_time.c)
