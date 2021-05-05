#include "connection_manager.h"
#include "LittleFS.h"
#include <time.h>

ConnectionManager::ConnectionManager(const char* ssid, const char* password)
  : m_ssid(ssid), m_password(password)
{

  
}

int8_t ConnectionManager::initialize() 
{
  Serial.println("Initializing connection manager");
  
  // connect to network
  int8_t const connection_status = connect();
  if (connection_status != WL_CONNECTED) {
    return connection_status;
  }
  
  // set clock
  if (!set_clock()) {
    return WL_CONNECT_FAILED;
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: "); 
  Serial.println(WiFi.localIP());
  
  return WL_CONNECTED;
}


bool ConnectionManager::load_certs() 
{
  static const char* PROGMEM CA_CERT_FILE = "/mosquitto-ca.crt";
  static const char* PROGMEM CLIENT_CERT_FILE = "/mqtt-client.crt";
  static const char* PROGMEM CLIENT_KEY_FILE = "/mqtt-client.key"; 

  Serial.println(); 
  Serial.println("Loading certificates...");
  
  if (!LittleFS.begin()) {
    Serial.println(F("Failed to mount the file system."));
    return false;
  }
  
  // CA certificate
  File hf = LittleFS.open(CA_CERT_FILE,"r");
  if (!hf) {
    Serial.printf("Failed to read %s\n",CA_CERT_FILE);
    LittleFS.end(); // unmount the file system
    return false;
  } else {
    String data = hf.readString();
    Serial.printf("Read certificate file %s\n",CA_CERT_FILE);
    //Serial.printf("%s!!!\n",data.c_str());
    if (!m_ca_cert.append(data.c_str())) {
      Serial.println("!!Failed to load CA certificate in list");
    } else {
      Serial.printf("CA certificate load OK, list count %d\n",m_ca_cert.getCount());
    }
    
    hf.close();
  }

/*
  // Client certificate
  hf = LittleFS.open(CLIENT_CERT_FILE,"r");
  if (!hf) {
    Serial.printf("Failed to read %s\n",CLIENT_CERT_FILE);
    LittleFS.end(); // unmount the file system
    return false;
  } else {
    String data = hf.readString();
    Serial.printf("Read certificate file %s\n",CLIENT_CERT_FILE);
    Serial.println(data);
    m_client_cert = BearSSL::X509List(data.c_str());
    
    hf.close();
  }

  // Client private key
  hf = LittleFS.open(CLIENT_KEY_FILE,"r");
  if (!hf) {
    Serial.printf("Failed to read %s\n",CLIENT_KEY_FILE);
    LittleFS.end(); // unmount the file system
    return false;
  } else {
    String data = hf.readString();
    Serial.printf("Read certificate file %s\n",CLIENT_KEY_FILE);
    Serial.println(data);
    m_client_key = BearSSL::PrivateKey(data.c_str());
    
    hf.close();
  }
*/
  m_client.setTrustAnchors(&m_ca_cert);
  //m_client.setClientRSACert(&m_client_cert, &m_client_key);

  LittleFS.end(); // unmount the file system
  return true;
}


int8_t ConnectionManager::connect() 
{
  Serial.println(); 
  Serial.print("Connecting to ");
  Serial.println(m_ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(m_ssid, m_password);

  return WiFi.waitForConnectResult();
}

bool ConnectionManager::set_clock() 
{
  Serial.println(); 
  Serial.println("Acquiring current time via NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.windows.com");

  static int8_t MAX_SAFETY_COUNT = 30; // seconds

  time_t now; 
  int8_t safety_count = 0;
  do {
    delay(1000);
    now = time(nullptr);  // this directly queries an NTP server
    ++safety_count;
  } while ((safety_count < MAX_SAFETY_COUNT) && (now < 24*3600));

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.println(asctime(&timeinfo));

  return (safety_count < MAX_SAFETY_COUNT);
}
