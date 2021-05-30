
#ifndef CONNECTIONMANAGER_H_
#define CONNECTIONMANAGER_H_

#include "ESP8266WiFi.h"

#include <time.h>

class ConnectionManager
{
public:
  ConnectionManager(const char* ssid, const char* password);

  bool load_certs();
  int8_t initialize();
  WiFiClientSecure& wifi_client() { return m_client; }

  bool sync_clock();
  time_t last_sync_time() { return m_last_sync_time; }
  
protected:

  int8_t connect();
  
  
private:
  const char* m_ssid;
  const char* m_password;

  BearSSL::WiFiClientSecure m_client;
  BearSSL::X509List m_ca_cert;
  BearSSL::X509List m_client_cert;
  BearSSL::PrivateKey m_client_key;

  time_t m_last_sync_time;
  
};


#endif //CONNECTIONMANAGER_H_
