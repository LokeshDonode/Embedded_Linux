#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <ArduinoMqttClient.h>

// ================= BMP280 =================
#define BMP280_ADDRESS 0x77
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BMP280 bmp;

// Sensor variables
float temperature;
float pressure;
float altitude;

// ================= WiFi =================
const char* ssid = "LokeshD_2.4G";
const char* password = "Lokesh@001";

// ================= MQTT =================
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "192.168.1.4";
int port = 1883;

// MQTT Topics
const char topicTemp[] = "sensor/bmp280/temperature";
const char topicPressure[] = "sensor/bmp280/pressure";
const char topicAltitude[] = "sensor/bmp280/altitude";

// Publish interval
const long interval = 5000;
unsigned long previousMillis = 0;

// =====================================================

void connectWiFi()
{
  Serial.println("=================================");
  Serial.print("Connecting to WiFi : ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected Successfully");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());
  Serial.println("=================================");
}

void connectMQTT()
{
  Serial.print("Connecting to MQTT Broker : ");
  Serial.println(broker);

  while (!mqttClient.connect(broker, port))
  {
    Serial.print("MQTT Connection Failed! Error Code = ");
    Serial.println(mqttClient.connectError());

    Serial.println("Retrying MQTT connection in 5 seconds...");
    delay(5000);
  }

  Serial.println("MQTT Connected Successfully!");
  Serial.println("=================================");
}

void setupBMP280()
{
  Serial.println("Initializing BMP280 Sensor...");

  if (!bmp.begin(BMP280_ADDRESS))
  {
    Serial.println("ERROR: BMP280 Sensor Not Found!");
    Serial.println("Check Wiring or I2C Address.");

    while (1)
    {
      delay(1000);
    }
  }

  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );

  Serial.println("BMP280 Initialization Successful");
  Serial.println("=================================");
}

void publishMQTT(const char* topic, float value)
{
  mqttClient.beginMessage(topic);
  mqttClient.print(value);
  mqttClient.endMessage();

  Serial.print("Published -> ");
  Serial.print(topic);
  Serial.print(" : ");
  Serial.println(value);
}

// =====================================================

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\nESP32 BMP280 MQTT Example");

  // Initialize modules
  connectWiFi();
  connectMQTT();
  setupBMP280();
}

// =====================================================

void loop()
{
  // Maintain MQTT connection
  mqttClient.poll();

  if (!mqttClient.connected())
  {
    Serial.println("MQTT Disconnected!");
    connectMQTT();
  }

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;

    // Read Sensor Data
    temperature = bmp.readTemperature();
    pressure = bmp.readPressure() / 100.0F;
    altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);

    // ================= SERIAL LOGS =================
    Serial.println("=========== BMP280 DATA ===========");

    Serial.print("Temperature : ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Pressure    : ");
    Serial.print(pressure);
    Serial.println(" hPa");

    Serial.print("Altitude    : ");
    Serial.print(altitude);
    Serial.println(" meters");

    Serial.println("===================================");

    // ================= MQTT PUBLISH =================
    publishMQTT(topicTemp, temperature);
    publishMQTT(topicPressure, pressure);
    publishMQTT(topicAltitude, altitude);

    Serial.println();
  }
}
