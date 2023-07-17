#include <ESP8266WiFi.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

Adafruit_MPU6050 mpu;
WiFiClient espClient;
PubSubClient client(espClient);

static const int RXPin = D7, TXPin = D6;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps; 
SoftwareSerial ss(TXPin, RXPin);

const char* ssid = "*************";
const char* password = "*************";
const char* passwordmqtt = "*************";
const char* token = "*************";
const char* mqttServer = "armadillo.rmq.cloudamqp.com";
int port = 1883;

const char* topicdata = "tfd/data";

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), token, passwordmqtt)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 5 seconds before retrying
      delay(2000);
    }
  }
}

void getData() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  String result = "";

  double ax = (a.acceleration.x);
  double ay = (a.acceleration.y);
  double az = (a.acceleration.z);
  double gx = (g.gyro.x);
  double gy = (g.gyro.y);
  double gz = (g.gyro.z);
  
  char axBuffer[20];
  char ayBuffer[20];
  char azBuffer[20];
  char gxBuffer[20];
  char gyBuffer[20];
  char gzBuffer[20];

  dtostrf(ax, 0, 6, axBuffer);
  dtostrf(ay, 0, 6, ayBuffer);
  dtostrf(az, 0, 6, azBuffer);
  dtostrf(gx, 0, 6, gxBuffer);
  dtostrf(gy, 0, 6, gyBuffer);
  dtostrf(gz, 0, 6, gzBuffer);

  double latitude = (gps.location.lat());     //Storing the Lat. and Lon.
  double longitude = (gps.location.lng());

  char latBuffer[20];
  char lonBuffer[20]; 
  
  dtostrf(latitude, 10, 6, latBuffer);  // Mengkonversi latitude menjadi string dengan 9 digit total dan 6 angka di belakang koma
  dtostrf(longitude, 10, 6, lonBuffer);  // Mengkonversi longitude menjadi string dengan 9 digit total dan 6 angka di belakang koma

  Serial.print("LAT:  ");
  Serial.println(latBuffer);  
  Serial.print("LONG: ");
  Serial.println(lonBuffer);


//  strcpy(payload, "");  
//  sprintf(payload, "%s,%s", latBuffer, lonBuffer);
//  return result = String(a.acceleration.x) + "/" + String (a.acceleration.y) + "/" + String (a.acceleration.z) + "/" + String(g.gyro.x) + "/" + String (g.gyro.y) + "/" + String (g.gyro.z);
//  return result = "Ax:" + String(a.acceleration.x) + "/Ay:" + String (a.acceleration.y) + "/Az:" + String (a.acceleration.z) + "/Gx:" + String(g.gyro.x) + "/Gy:" + String (g.gyro.y) + "/Gz:" + String (g.gyro.z)+"/"+ String(payload);
//  return result = "Ax:" + String(a.acceleration.x) + "/Ay:" + String (a.acceleration.y) + "/Az:" + String (a.acceleration.z) + "/Gx:" + String(g.gyro.x) + "/Gy:" + String (g.gyro.y) + "/Gz:" + String (g.gyro.z)+"/lat:"+ latBuffer + "/long:" + lonBuffer;
//  result = "Ax:" + axBuffer + "/Ay:" + ayBuffer + "/Az:" + azBuffer + "/Gx:" + gxBuffer + "/Gy:" + gyBuffer + "/Gz:" + gzBuffer+"/lat:"+ latBuffer + "/long:" + lonBuffer;
    result = "Ax:" + String(axBuffer) + "/Ay:" + String(ayBuffer) + "/Az:" + String(azBuffer) + "/Gx:" + String(gxBuffer) + "/Gy:" + String(gyBuffer) + "/Gz:" + String(gzBuffer)+"/lat:"+ latBuffer + "/long:" + lonBuffer;

    Serial.println(result.c_str());
    client.publish(topicdata,result.c_str());
}


//void displayInfo(){
//  float latitude = (gps.location.lat());     //Storing the Lat. and Lon.
//  float longitude = (gps.location.lng());
//
//  char latBuffer[20];
//  char lonBuffer[20];
//
//  dtostrf(latitude, 10, 6, latBuffer);  
//  dtostrf(longitude, 10, 6, lonBuffer);  
//
//  Serial.print("LAT:  ");
//  Serial.println(latBuffer);  
//  Serial.print("LONG: ");
//  Serial.println(lonBuffer);
//
//  char payload[18];
//  strcpy(payload, "");  
//  sprintf(payload, "%s,%s", latBuffer, lonBuffer);
//  Serial.println(payload);
//
//  client.publish(topicloc, payload);
//}

void setup(void) {
  Serial.begin(9600);
  setup_wifi();
  ss.begin(GPSBaud);

  client.setServer(mqttServer, port);
  client.setCallback(callback);
  
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens
  Serial.println("Adafruit MPU6050 test!");
  
  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  client.subscribe(topicdata);
  Serial.println("");
  delay(100);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(10);
  
  while (ss.available() > 0)
  {
    // sketch displays information every time a new sentence is correctly encoded.
    if (gps.encode(ss.read()))
//      displayInfo();
      getData();  
  }
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    while (true);
  }
}
