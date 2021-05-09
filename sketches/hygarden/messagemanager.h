#ifndef messagemanager_h_
#define messagemanager_h_

#include <Arduino.h>

#define MAX_DISCOVERY_KEY_LEN 32
#define MAX_BUF_LEN 768

struct MessageManagerImpl;
class Adafruit_MQTT_Client;
class HyGardenConfig;
class SensorsManager;

class MessageManager
{
  public:
    MessageManager();
    ~MessageManager();

    void initialize(Adafruit_MQTT_Client& mqtt_client);

    static void updateSensorsMonitor(HyGardenConfig const& config, SensorsManager& monitor, 
      HyGardenConfig const* old_config = nullptr);

    void poll_subscriptions(HyGardenConfig& config, SensorsManager& monitor);
    void publish_sensor_data(HyGardenConfig const& config, SensorsManager const& monitor);
    void keep_alive();

  private:
    void on_discovery(char* data, uint16_t const len, HyGardenConfig& config);
    void on_control(char* data, uint16_t const len, HyGardenConfig& config, SensorsManager& monitor);
    
    uint32_t get_elapsed_time() const;
    void reset_keep_alive();
    uint32_t last_keep_alive_ms_;

    MessageManagerImpl* impl_;

};

#endif // messagemanager_h_
