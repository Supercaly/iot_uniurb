#include "wifi.h"

#include <WiFi.h>
#include "IPAddress.h"

bool wifi_is_connected() {
  return (WiFi.status() == WL_CONNECTED);
}

bool wifi_connect(const char *ssid, const char *pwd, int max_retry, int pause) {
  LOGGER_SERIAL.println("wifi_connect: connecting to SSID: " + String(ssid)
                        + " with pwd: " + String(pwd)
                        + " attempt: " + String(max_retry)
                        + " delay: " + String(pause));

  LED_ON(SIGNAL_LED_PIN);

  // Enable WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);
  LOGGER_SERIAL.println("WiFi: board MAC Address '" + WiFi.macAddress() + "'");
  LOGGER_SERIAL.println("WiFi: connecing to WiFi");

  // Test the connection for some times
  int conn_attempt = 0;
  while (!wifi_is_connected() && conn_attempt < max_retry) {
    LOGGER_SERIAL.println(".");
    delay(pause);
    conn_attempt++;
  }

  // If we are not connected return failure
  if (conn_attempt >= max_retry) {
    LOGGER_SERIAL.println("failed");
    return false;
  }

  // Connection done
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  LOGGER_SERIAL.println("done");

  LED_OFF(SIGNAL_LED_PIN);
  return true;
}

String wifi_get_ip() {
  return WiFi.localIP().toString();
}