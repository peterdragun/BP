/**
* @file  wifi_connect.h
*
* @brief Handlers wifi connections
* @author Peter Dragun (xdragu01)
*/

#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_wifi_default.h"
#include "esp_wifi.h"

#define TAG "wifi"
#define GOT_IPV4_BIT BIT(0)
#define GOT_IPV6_BIT BIT(1)

#define CONNECTED_BITS (GOT_IPV4_BIT | GOT_IPV6_BIT)

#if defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_LOCAL_LINK)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_LINK_LOCAL
#elif defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_GLOBAL)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_GLOBAL
#elif defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_SITE_LOCAL)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_SITE_LOCAL
#elif defined(CONFIG_EXAMPLE_CONNECT_IPV6_PREF_UNIQUE_LOCAL)
#define EXAMPLE_CONNECT_PREFERRED_IPV6_TYPE ESP_IP6_ADDR_IS_UNIQUE_LOCAL
#endif // if-elif CONFIG_EXAMPLE_CONNECT_IPV6_PREF_...

// global variables
extern char wifi_ssid[32];
extern char wifi_pass[64];

/**
 * @brief Connect to wifi, store IP address
 * 
 * @return ESP_OK on successful connection
 */
esp_err_t wifi_connect(void);

#endif //WIFI_CONNECT_H
