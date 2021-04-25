#include "connection_manager.h"
#include "secrets.h"

// Global objects
ConnectionManager conn (WLAN_SSID, WLAN_PASS);
WiFiServer server(80);  // Set web server port number to 80
String header;          // Variable to store the HTTP request


// Auxiliary variables to store the current output state
String ledState = "off";

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


// N times: off (T_low), on (T_high)
// finished state ON
void blinky(int T_high, int T_low, int N) {
  for (int i = 0; i < N; i++) {
    digitalWrite(BUILTIN_LED, HIGH);
    delay(T_low);
    digitalWrite(BUILTIN_LED, LOW);
    delay(T_high);
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  
  Serial.begin(115200);
  delay(10);

  // blinky 2s, ON at end
  blinky(160,40,10);
  
  Serial.println(F("Wifi Connection Example"));
  delay(1000);

  while (conn.initialize() != WL_CONNECTED) {
    Serial.println(F("Failed to connect to network. Restarting..."));
    
    blinky(200,100,2);
    delay(5000);
    blinky(200,100,2);
    
    ESP.restart();
  }

  server.begin();

  blinky(200,100,4);
  digitalWrite(BUILTIN_LED, HIGH);  // off, connected


  delay(500);


}


// https://randomnerdtutorials.com/esp8266-web-server/
void loop() {
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /led/on") >= 0) {
              Serial.println("LED on");
              output5State = "on";
              digitalWrite(BUILTIN_LED, LOW);
            } else if (header.indexOf("GET /led/off") >= 0) {
              Serial.println("LED off");
              output5State = "off";
              digitalWrite(BUILTIN_LED, HIGH);
            } 
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP8266 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 5  
            client.println("<p>LED - State " + ledState + "</p>");
            // If the output5State is off, it displays the ON button       
            if (ledState=="off") {
              client.println("<p><a href=\"/led/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/led/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

}
