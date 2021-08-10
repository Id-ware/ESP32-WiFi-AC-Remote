/*********
  WiFi AC Web Controller example using IRremoteESP8266 and IRsend
  
  based on example by Rui Santos
  https://randomnerdtutorials.com/esp32-web-server-arduino-ide/
  And IRremoteESP8266's IRsendDemo Example sketch form:
  https://github.com/crankyoldgit/IRremoteESP8266
  https://github.com/crankyoldgit/IRremoteESP8266/tree/master/examples/IRsendDemo
  
  esp IR remote codes dumper/reader can be found here:
  https://community.dfrobot.com/makelog-308342.html
  
*********/

// Load Wi-Fi library
#include <WiFi.h>

// load IR library
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// Example of AC remote raw data captured 
// you need to capture your own AC remote code! (follow capturing instructions on https://community.dfrobot.com/makelog-308342.html): 
uint16_t rawData[163] = {9058,4438,512,1728,512,1726,512,610,510,608,510,610,
                         510,608,510,608,510,1728,510,610,510,1728,510,1728,510,610,510,608,
                         510,610,510,610,510,608,510,610,510,608,510,1728,512,608,510,610,510,
                         608,510,610,510,610,510,610,510,1730,510,610,510,610,510,608,510,610,608,
                         510,608,510,610,510,608,510,608,510,608,510,608,510,608,510,608,510,610,
                         510,608,510,608,510,610,510,608,510,608,510,608,510,608,510,610,510};

// Replace with your network credentials
const char* ssid = "******";
const char* password = "******";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String acState = "off";

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

// wifi loggin attempts test
int wifiloginattempts = 0; 

void setup() {
  irsend.begin();
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); 
  
  //wifi loggin attempts test
  wifiloginattempts = 0;
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiloginattempts++;  //if not logged in yet, counts the attempts
  if (wifiloginattempts >= 24) { 
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
    }  
  }
  
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){
  
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
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
            if (header.indexOf("GET /ac/on") >= 0) {
              Serial.println("AC on");
              acState = "on";
              AcOn();
              //digitalWrite(output26, HIGH);
            } else if (header.indexOf("GET /ac/off") >= 0) {
              Serial.println("AC off");
              acState = "off";
              AcOff();
              //digitalWrite(output26, LOW);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>AC WiFi Controller (via ESP32)</h1>");
            
            // Display current state, and ON/OFF buttons for the AC (GPIO 26)  
            client.println("<p>AC - State " + acState + "</p>");
            // If the acState is off, it displays the ON button       
            if (acState=="off") {
              client.println("<p><a href=\"/ac/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/ac/off\"><button class=\"button button2\">OFF</button></a></p>");
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

void AcOn() {
  irsend.sendRaw(rawData, 163, 38);  // Send a raw data capture at 38kHz.

  //experimental 
  //i ended up not using it, if you want to try it, find your ac vendor in the IRsend.h and IRsend.cpp file
  //https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/IRsend.h
  //https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/IRsend.cpp
  
  //irsend.sendAirwell(0xFD00FF);
}

void AcOff() {
  irsend.sendRaw(rawData, 163, 38);  // Send a raw data capture at 38kHz.

  //experimental 
  //i ended up not using it, if you want to try it, find your ac vendor in the IRsend.h and IRsend.cpp file
  //https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/IRsend.h
  //https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/IRsend.cpp
  
  //irsend.sendAirwell(0xFD00FF);
}
