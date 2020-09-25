#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <PMserial.h>

#define DHTPIN 23

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

SerialPM pms(PMSx003, Serial);  // PMSx003, RX, TX
DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

const char* ssid = "";
const char* password = "";
WiFiServer server(9100);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(9600);
  pms.init();                   // config serial port

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  // Initialize device.
  dht.begin();
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;

}

void loop() {
  delay(500);                 // wait for half second

  // WIFI server
  WiFiClient client = server.available();
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
            if (header.indexOf("GET /metrics") >= 0) { // serve metrics page
              pms.read();                   // read the PM sensor

              Serial.print(F("\nPM1.0 "));Serial.print(pms.pm01);Serial.print(F(", "));
              Serial.print(F("PM2.5 "));Serial.print(pms.pm25);Serial.print(F(", "));
              Serial.print(F("PM10 ")) ;Serial.print(pms.pm10);Serial.println(F(" [ug/m3]"));

              sensors_event_t event;
              dht.temperature().getEvent(&event);
              float temperature = event.temperature;
              dht.humidity().getEvent(&event);
              float relative_humidity = event.relative_humidity;

              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-Encoding: identity"));
              client.println(F("Content-Type: text/plain; version=0.0.4; charset=utf-8"));
              client.println(F("Date: Wed, 16 Sep 2020 07:00:19 GMT"));
              client.println();

              client.print(F("# HELP room_temperature The temperature from the sensor.\n"));
              client.print(F("# TYPE room_temperature gauge\n"));
              client.print(String("room_temperature ") + String(temperature) + String("\n"));
              client.print(F("# HELP room_relative_humidity The humidity from the sensor.\n"));
              client.print(F("# TYPE room_relative_humidity gauge\n"));
              client.print(String("room_relative_humidity ") + String(relative_humidity) + String("\n"));

              client.print(F("# HELP room_pm01 The pm1.0 from the sensor.\n"));
              client.print(F("# TYPE room_pm01 gauge\n"));
              client.print(String("room_pm01 ") + String(pms.pm01) + String("\n"));
              client.print(F("# HELP room_pm25 The pm2.5 from the sensor.\n"));
              client.print(F("# TYPE room_pm25 gauge\n"));
              client.print(String("room_pm25 ") + String(pms.pm25) + String("\n"));

              client.print(F("# HELP room_pm10 The pm10 from the sensor.\n"));
              client.print(F("# TYPE room_pm10 gauge\n"));
              client.print(String("room_pm10 ") + String(pms.pm10) + String("\n"));
            } else { // serve index page
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type: text/html; charset=utf-8"));
              client.println();

              // Display the HTML web page
              client.println(F("<!DOCTYPE html><html>"));
              client.println(F("<head><title>Room Environmental Sensor</title></head>"));

              // Web Page Heading
              client.println(F("<body><h1>Room Environmental Sensor</h1><p><a href=\"/metrics\">Metrics</a></p>"));
              client.println(F("</body></html>"));

              // The HTTP response ends with another blank line
              client.println();
            }
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

