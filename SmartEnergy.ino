#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>co
#include <Wire.h>
#include "Adafruit_BMP280.h"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define PIR_PIN 14  // Digital pin connected to the PIR sensor
#define PIEZO_PIN 32 // Analog pin connected to the piezo sensor
#define BUZZER_PIN 33 // Digital pin connected to the buzzer

#define WIFI_SSID "restless"
#define WIFI_PASSWORD "studywell"
#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com/"
#define INFLUXDB_TOKEN "JWdb8fii7jHcoPt0mCY2clzE5kxp6I7chuzgnwMI9TzG0juUuJR6DHYdWJZF3Fn7-IkdkanjYohnb9uiPmg6qw=="
#define INFLUXDB_ORG "personal"
#define INFLUXDB_BUCKET "test"
#define INFLUXDB_MEASUREMENT "esp32_1_sensors"

WiFiClientSecure net;

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensor(INFLUXDB_MEASUREMENT);

Adafruit_BMP280 bmp;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.println("Connecting to Wi-Fi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void get_BMP280_sensor_data() {
  Serial.println();
  Serial.println("-----get_BMP280_sensor_data()");
  
  float temp = bmp.readTemperature();
  float pres = bmp.readPressure();
  float alt = bmp.readAltitude(); // Read altitude
  
  // Check if the temperature reading is valid
  if (pres<0) {
    Serial.println("Temperature reading is NULL. Ringing buzzer.");
    digitalWrite(BUZZER_PIN, HIGH); // Ring the buzzer
    delay(1000); // Keep the buzzer on for 1 second
    digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer
  } else {
    Serial.printf("Temperature : %.2f °C\n", temp);
    Serial.printf("Pressure : %.2f Pa\n", pres);
    Serial.printf("Altitude : %.2f m\n", alt);
    Serial.println("-------------");

    sensor.clearFields();
    sensor.addField("temperature", temp);
    sensor.addField("pressure", pres);
    sensor.addField("altitude", alt);
  }
}

void setup() {
  Serial.begin(9600);
  connectWiFi();
  #define TZ_INFO "IST-5:30"  // Define your timezone here

  // Initialize BMP280 sensor
  int status = bmp.begin(0x76, 0x58);
  if (!status) {
    Serial.println("Error initializing BMP280 sensor");
    while (1);
  }

  // Accurate time is necessary for certificate validation and writing in batches
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT); // Initialize the buzzer pin as an output
}

void loop() {
  int motionDetected = digitalRead(PIR_PIN);
  Serial.print(F("Motion Detected: "));
  Serial.println(motionDetected);
  
  get_BMP280_sensor_data();

  // Reading value from piezo 
  int piezoValue = analogRead(PIEZO_PIN);
  Serial.print("Piezo Sensor Value: ");
  Serial.println(piezoValue);

  sensor.clearTags();
  sensor.addTag("location", "living_room");
  sensor.addField("motion_detected", motionDetected);
  sensor.addField("piezo_value", piezoValue);

  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  delay(1000);
} 
