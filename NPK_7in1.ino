#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>

// -------------------- PIN DEFINITIONS --------------------
#define RX_PIN D1
#define TX_PIN D2
#define RELAY1_PIN D8

// -------------------- THINGSPEAK CONFIGURATION --------------------
const char *thingSpeakServer = "api.thingspeak.com";
WiFiClient client;

unsigned long writeChannelID = 2581715;
const char *writeAPIKey = "SG59416ORUUOFN1R";

unsigned long readChannelID = 2586515;
const char *readAPIKey = "KTPPICJ0W1RSNV1L";
unsigned int buttonField = 1; // Field untuk button kendali relay

// -------------------- SERIAL CONFIG --------------------
SoftwareSerial soilSensorSerial(RX_PIN, TX_PIN); // RX, TX

// -------------------- SETUP --------------------
void setup() {
Serial.begin(9600);
soilSensorSerial.begin(4800);

pinMode(RELAY1_PIN, OUTPUT);
digitalWrite(RELAY1_PIN, LOW); // Awal relay mati

// ---------------- WiFiManager Setup ----------------
WiFiManager wifiManager;
wifiManager.setTimeout(180); // Timeout 3 menit untuk koneksi
if (!wifiManager.autoConnect("SensorNPK_AP")) {
Serial.println("Gagal terkoneksi. Restart ESP...");
ESP.restart();
delay(1000);
}

Serial.println("WiFi Tersambung!");
Serial.print("IP Address: ");
Serial.println(WiFi.localIP());

ThingSpeak.begin(client);
}

// -------------------- LOOP --------------------
void loop() {
byte queryCommand[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08};
byte responseBuffer[19];

soilSensorSerial.write(queryCommand, sizeof(queryCommand));
delay(1000);

if (soilSensorSerial.available() >= sizeof(responseBuffer)) {
soilSensorSerial.readBytes(responseBuffer, sizeof(responseBuffer));

unsigned int soilHumidity     = (responseBuffer[3]  << 8) | responseBuffer[4];
unsigned int soilTemperature  = (responseBuffer[5]  << 8) | responseBuffer[6];
unsigned int conductivity     = (responseBuffer[7]  << 8) | responseBuffer[8];
unsigned int soilPH           = (responseBuffer[9]  << 8) | responseBuffer[10];
unsigned int nitrogen         = (responseBuffer[11] << 8) | responseBuffer[12];
unsigned int phosphorus       = (responseBuffer[13] << 8) | responseBuffer[14];
unsigned int potassium        = (responseBuffer[15] << 8) | responseBuffer[16];

Serial.println("===== Data Sensor Tanah =====");
Serial.printf("Kelembaban Tanah     : %.1f %%\n", soilHumidity / 10.0);
Serial.printf("Suhu Tanah           : %.1f °C\n", soilTemperature / 10.0);
Serial.printf("Konduktivitas        : %u uS/cm\n", conductivity);
Serial.printf("pH Tanah             : %.1f\n", soilPH / 10.0);
Serial.printf("Nitrogen             : %u mg/kg\n", nitrogen);
Serial.printf("Fosfor               : %u mg/kg\n", phosphorus);
Serial.printf("Kalium               : %u mg/kg\n", potassium);
Serial.println("==============================");

// Kirim data ke ThingSpeak
ThingSpeak.setField(1, soilHumidity / 10.0);
ThingSpeak.setField(2, soilTemperature / 10.0);
ThingSpeak.setField(3, conductivity);
ThingSpeak.setField(4, soilPH / 10.0);
ThingSpeak.setField(5, nitrogen);
ThingSpeak.setField(6, phosphorus);
ThingSpeak.setField(7, potassium);

int writeStatus = ThingSpeak.writeFields(writeChannelID, writeAPIKey);
if (writeStatus == 200) {
Serial.println("Data berhasil dikirim ke ThingSpeak.");
} else {
Serial.print("Gagal mengirim data. Kode: ");
Serial.println(writeStatus);
}
}

// Baca status tombol kendali relay dari ThingSpeak
long buttonStatus = ThingSpeak.readLongField(readChannelID, buttonField, readAPIKey);
int readStatus = ThingSpeak.getLastReadStatus();

if (readStatus == 200) {
Serial.println("Status Tombol: " + String(buttonStatus));
digitalWrite(RELAY1_PIN, buttonStatus == 1 ? HIGH : LOW);
} else {
Serial.println("Gagal membaca tombol. Kode: " + String(readStatus));
}

delay(1000);
}