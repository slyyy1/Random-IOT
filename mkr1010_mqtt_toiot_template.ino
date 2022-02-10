#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Arduino_MKRENV.h>


// --------------------------------------------------------------------------------------------
//        UPDATE CONFIGURATION TO MATCH YOUR ENVIRONMENT
// --------------------------------------------------------------------------------------------

// Watson IoT connection details
#define MQTT_HOST "07j0fn.messaging.internetofthings.ibmcloud.com"
#define MQTT_PORT 1883
#define MQTT_DEVICEID "d:07j0fn:mkr1010:4c11ae91134c"
#define MQTT_USER "use-token-auth"
#define MQTT_TOKEN "mkr1010_4c11ae91134c"
#define MQTT_TOPIC "iot-2/evt/status/fmt/json"
#define MQTT_TOPIC_DISPLAY "iot-2/cmd/display/fmt/json"

// Add WiFi connection information
char ssid[] = "YYJ";     //  your network SSID (name) ABC
char pass[] = "yyjyyjyyj";  // your network password 123456789

int WiFistatus = WL_IDLE_STATUS;     // the Wifi radio's status

// --------------------------------------------------------------------------------------------
//        SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS LINE
// --------------------------------------------------------------------------------------------

// MQTT objects
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, wifiClient);

// variables to hold data
StaticJsonDocument<500> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
JsonObject status = payload.createNestedObject("d");
static char msg[500];

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");

  payload[length] = 0; // ensure valid content is zero terminated so can treat as c-string
  Serial.println((char *)payload);
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  if (!ENV.begin()) { //check correct initialisation of shield
    Serial.println("Failed to initialize MKR ENV shield!");
    while (1); //infinite loop, avoids loop to be executed
  }
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // attempt to connect to Wifi network:

  while (WiFistatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    WiFistatus = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");
  
  Serial.println("----------------------------------------");
  printData();
  Serial.println("----------------------------------------");

  // Connect to MQTT - IBM Watson IoT Platform
  if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
    Serial.println("MQTT Connected");
    mqtt.subscribe(MQTT_TOPIC_DISPLAY);

  } else {
    Serial.println("MQTT Failed to connect!");
  }
}

void loop() {
  mqtt.loop();
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
      Serial.println("MQTT Connected");
      mqtt.subscribe(MQTT_TOPIC_DISPLAY);
      mqtt.loop();
    } else {
      Serial.println("MQTT Failed to connect!");
      delay(5000);
    }
  }
  //float temperature = 24.5;
   // read all the sensor values using the library functions
  float temperature = ENV.readTemperature();
  float humidity    = ENV.readHumidity();
  float pressure    = ENV.readPressure();
  float illuminance = ENV.readIlluminance();
  float uva         = ENV.readUVA();
  float uvb         = ENV.readUVB();
  float uvIndex     = ENV.readUVIndex();

  // Check if any reads failed and exit early (to try again).
  if (isnan(temperature)) {
    Serial.println("Failed to read data!");
  } else {

    // Send data to Watson IoT Platform
    status["Temperature"] = temperature;
    status["Humidity"] = humidity;
    status["Pressure"] = pressure;
    status["Illuminance"] = illuminance;
    status["Uva"] = uva;
    status["Uvb"] = uvb;
    status["UvIndex"] = uvIndex;
    
    serializeJson(jsonDoc, msg, 500);
    Serial.println(msg);
    if (!mqtt.publish(MQTT_TOPIC, msg)) {
      Serial.println("MQTT Publish failed");
    }
  }

  // Pause - but keep polling MQTT for incoming messages
  for (int i = 0; i < 10; i++) {
    mqtt.loop();
    delay(1000);
  }
}

void printData() {
  Serial.println("Board Information:");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.println();
  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}
