#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager
#include <WiFiClientSecure.h> // Required for TLS/SSL (port 8883)
#include <PubSubClient.h>     // https://github.com/knolleary/pubsubclient

// --- MQTT Configuration ---
#define MQTT_HOST "e673f5c9733d4189b13d6738677730b2.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "DemoUser"
#define MQTT_PASS "A12341234a!"
#define CONTROL_TOPIC "esp32/led/control"
#define STATUS_TOPIC "esp32/led/status"
#define LED_PIN 2 // Built-in LED on most ESP32 dev boards

// --- Global Objects ---
WiFiClientSecure espClient;
PubSubClient client(espClient);
long lastMsg = 0;

// --- Function Prototypes ---
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnect();

/**
 * @brief Handles incoming MQTT messages and controls the LED.
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Check if the message is for the LED control topic
  if (String(topic).equals(CONTROL_TOPIC)) {
    if (message == "0") {
      digitalWrite(LED_PIN, LOW); // LED is typically active LOW on ESP32
      Serial.println("LED turned ON.");
      // Publish status update
      client.publish(STATUS_TOPIC, "ON");
    } else if (message == "1") {
      digitalWrite(LED_PIN, HIGH); // LED is typically active LOW on ESP32
      Serial.println("LED turned OFF.");
      // Publish status update
      client.publish(STATUS_TOPIC, "OFF");
    } else {
      Serial.println("Invalid payload. Send '1 - OFF' or '0 - ON'.");
    }
  }
}

/**
 * @brief Attempts to connect to the MQTT broker.
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect with username and password
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("connected to MQTT broker.");
      // Once connected, subscribe to the control topic
      client.subscribe(CONTROL_TOPIC);
      Serial.print("Subscribed to: ");
      Serial.println(CONTROL_TOPIC);
      
      // Publish initial status
      client.publish(STATUS_TOPIC, "READY");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // Initialize Serial and LED pin
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Start with LED OFF (assuming active LOW)

  // --- WiFi Connection using WiFiManager (from your reference code) ---
  WiFi.mode(WIFI_STA);
  WiFiManager wm;

  // Set the ESP32 to skip SSL certificate verification (needed for this demo on HiveMQ Cloud)
  // WARNING: DO NOT use setInsecure() in a production environment!
  espClient.setInsecure();

  // Configure MQTT client
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(mqttCallback);

  Serial.println("Starting WiFi connection...");
  bool res = wm.autoConnect("AutoConnectAP", "password");

  if (!res) {
    Serial.println("Failed to connect to WiFi. Restarting...");
    // In a real application, you might want a more graceful failure here
    // ESP.restart(); 
  } else {
    Serial.println("Connected to WiFi successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
}

void loop() {
  // Check and maintain MQTT connection
  if (!client.connected()) {
    reconnect();
  }
  
  // MQTT client loop (must be called frequently to process incoming messages)
  client.loop();

  // Optional: Publish a status message every 60 seconds (60000ms)
  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;
    client.publish(STATUS_TOPIC, "Heartbeat");
  }
}