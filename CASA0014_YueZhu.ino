#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "arduino_secrets.h"

// Sensor pin and type
#define DHTPIN 0          // DHT-11 connected to pin D0
#define DHTTYPE DHT11     // Using DHT-11 sensor
DHT dht(DHTPIN, DHTTYPE);

// WiFi and MQTT configuration
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server = SECRET_MQTTSERVER;
const int mqtt_port = SECRET_MQTTPORT;

// MQTT client setup
WiFiClient mkrClient;
PubSubClient client(mkrClient);

// Chrono Lumina topic
char mqtt_topic_demo[] = "student/CASA0014/light/7/pixel/";

// Thresholds and sensitivity settings
const float TEMP_HIGH_THRESHOLD = 30.0;
const float TEMP_LOW_THRESHOLD = 15.0;
const float HUMIDITY_THRESHOLD = 100.0;
const int SOUND_SENSITIVITY = 50; // Increase sound sensitivity

// Sound sensor setup
#define SOUND_PIN A0
int lastSoundLevel = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Connect to WiFi
  connectToWiFi();

  // Set up MQTT server
  client.setServer(mqtt_server, mqtt_port);
  reconnectMQTT();

  Serial.println("Setup complete");
}

void loop() {
  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Read sound intensity
  int soundLevel = analogRead(SOUND_PIN);
  int soundChange = abs(soundLevel - lastSoundLevel);
  lastSoundLevel = soundLevel;

  // Debugging output
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C | Humidity: ");
  Serial.print(humidity);
  Serial.print(" % | Sound Change: ");
  Serial.println(soundChange);

  // Dynamic light effects
  sendDynamicPixels(temperature, humidity, soundChange);

  delay(10); // Update frequency control
}

void sendDynamicPixels(float temperature, float humidity, int soundChange) {
  char mqtt_message[100];

  // Map temperature to red brightness
  int red = map(temperature, TEMP_LOW_THRESHOLD, TEMP_HIGH_THRESHOLD, 0, 255);
  red = constrain(red, 0, 255);

  // Map humidity to green brightness
  int green = map(humidity, 0, HUMIDITY_THRESHOLD, 0, 255);
  green = constrain(green, 0, 255);

  // Map sound change to blue brightness
  int blue = map(soundChange * SOUND_SENSITIVITY, 0, 1023, 0, 255);
  blue = constrain(blue, 0, 255);

  // Resolve red-green conflict
  if (red > 200 && green > 200) {
    if (red >= green) {
      green /= 2; // Prioritize reducing green
    } else {
      red /= 2; // Prioritize reducing red
    }
  }

  // Debugging output
  Serial.print("RGB Values -> R: ");
  Serial.print(red);
  Serial.print(", G: ");
  Serial.print(green);
  Serial.print(", B: ");
  Serial.println(blue);

  // Dynamic frame animation
  int totalFrames = 12;
  for (int frame = 1; frame <= totalFrames; frame++) {
    for (int i = 0; i < totalFrames; i++) {
      if (i < frame) {
        // Light up LEDs in the current range
        sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": 0}", i, red, green, blue);
      } else {
        // Turn off other LEDs
        sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": 0, \"G\": 0, \"B\": 0, \"W\": 0}", i);
      }
      client.publish(mqtt_topic_demo, mqtt_message);
      delay(20); // Frame interval
    }
    delay(50); // Frame transition delay
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "MKR1010-Client";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}
