
#ifndef CONNECTIONMANAGER_H_
#define CONNECTIONMANAGER_H_

#include "ESP8266WiFi.h"

class ConnectionManager
{
public:
  ConnectionManager(const char* ssid, const char* password);

  bool load_certs();
  int8_t initialize();
  WiFiClientSecure& wifi_client() { return m_client; }
  
protected:

  int8_t connect();
  bool set_clock();
  
private:
  const char* m_ssid;
  const char* m_password;

  BearSSL::WiFiClientSecure m_client;
  BearSSL::X509List m_ca_cert;
  BearSSL::X509List m_client_cert;
  BearSSL::PrivateKey m_client_key;
  
};


#endif //CONNECTIONMANAGER_H_
