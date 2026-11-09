#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// // ================= KONFIGURASI JARINGAN =================
// const char* ssid       = "TP-Link_96F2";
// const char* password   = "89508367";
// const char* serverUrl  = "http://192.168.0.100:4000/api/telemetry";
const char* ssid       = "Jojo";
const char* password   = "Jojoo1234";
const char* serverUrl  = "http://172.20.10.5:4000/api/telemetry"; 

// ================= DEFINISI PIN =================
#define DHT_PIN         4
#define DHT_TYPE        DHT11
#define SOIL_PIN        34
#define UV_PIN          35
#define LDR_PIN         32
#define SOIL_PWR        25
#define DS18B20_PIN     27

// ================= KONFIGURASI AKTUATOR & I2C =================
#define RELAY_LIGHT_PIN 26
#define RELAY_PUMP_PIN  13
#define I2C_SDA         21
#define I2C_SCL         22
#define LCD_ADDR        0x27

#define LIGHT_ACTIVE_LOW  false
#define LIGHT_LEVEL_ON   (LIGHT_ACTIVE_LOW ? LOW  : HIGH)
#define LIGHT_LEVEL_OFF  (LIGHT_ACTIVE_LOW ? HIGH : LOW)

#define PUMP_ACTIVE_LOW   true
#define PUMP_LEVEL_ON    (PUMP_ACTIVE_LOW ? LOW  : HIGH)
#define PUMP_LEVEL_OFF   (PUMP_ACTIVE_LOW ? HIGH : LOW)

// ================= KALIBRASI & AMBANG BATAS =================
#define SOIL_DRY        0
#define SOIL_WET        2450
#define BATAS_POMPA_ON   30
#define BATAS_POMPA_OFF  70
#define BATAS_LDR_ON     60
#define BATAS_LDR_OFF    80

// ================= INISIALISASI GLOBAL =================
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensorSuhuTanah(&oneWire);

bool pompaNyala = false;
bool lampuNyala = false;
String modeKontrol = "AUTO";

// Variabel untuk Non-Blocking Timer (millis)
unsigned long waktuSebelumnya = 0;
const long intervalLoop = 2000; 

// ================= FUNGSI BANTUAN =================
void tampilBaris(uint8_t baris, const char *teks) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", teks);
  lcd.setCursor(0, baris);
  lcd.print(buf);
}

int bacaTanahPersen() {
  digitalWrite(SOIL_PWR, HIGH);     
  delay(300);                      
  long total = 0;
  for (int i = 0; i < 10; i++) {    
    total += analogRead(SOIL_PIN);
    delay(10);
  }
  digitalWrite(SOIL_PWR, LOW);      
  int raw    = total / 10;
  int persen = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
  return constrain(persen, 0, 100);
}

const char *statusTanah(int persen) {
  if (persen < BATAS_POMPA_ON)   return "KERING";
  if (persen <= BATAS_POMPA_OFF) return "LEMBAB";
  return "BASAH";
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(1000);

  digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);
  pinMode(RELAY_LIGHT_PIN, OUTPUT);
  digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);

  digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);
  pinMode(RELAY_PUMP_PIN, OUTPUT);
  digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);

  pinMode(SOIL_PWR, OUTPUT);
  digitalWrite(SOIL_PWR, LOW);      

  Wire.begin(I2C_SDA, I2C_SCL);
  dht.begin();
  sensorSuhuTanah.begin();
  
  lcd.init();
  lcd.backlight();
  tampilBaris(0, "Smart Planter");
  tampilBaris(1, "Connecting WiFi");

  WiFi.begin(ssid, password);
  Serial.printf("Menghubungkan ke Wi-Fi: %s\n", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWi-Fi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  tampilBaris(1, "WiFi Connected");
  delay(2000);
  lcd.clear();
  Serial.println("=== SISTEM OTOMASI SMART INDOOR PLANTER AKTIF ===");
}

// ================= LOOP UTAMA =================
void loop() {
  unsigned long waktuSekarang = millis();

  if (waktuSekarang - waktuSebelumnya >= intervalLoop) {
    waktuSebelumnya = waktuSekarang; 

    // 1. PEMBACAAN SENSOR
    float suhuUdara  = dht.readTemperature();
    float kelembapan = dht.readHumidity();
    int   tanah      = bacaTanahPersen();
    
    sensorSuhuTanah.requestTemperatures(); 
    float suhuTanahRaw = sensorSuhuTanah.getTempCByIndex(0);
    bool suhuTanahValid = (suhuTanahRaw != DEVICE_DISCONNECTED_C) && (suhuTanahRaw > -10.0) && (suhuTanahRaw < 60.0);
    float suhuTanah = suhuTanahValid ? suhuTanahRaw : 0.0;
    
    int   ldrRaw     = analogRead(LDR_PIN);
    int   ldrPersen  = constrain(map(ldrRaw, 4095, 0, 0, 100), 0, 100);
    
    int   uvRaw      = analogRead(UV_PIN);
    float uvIndex    = ((uvRaw * 3.3) / 4095.0) / 0.1; 

    // 2 & 3. LOGIKA KONTROL OTOMATIS (Hanya jika Mode Kontrol = AUTO)
    if (modeKontrol == "AUTO") {
      // Logika Pompa Air
      if (tanah < BATAS_POMPA_ON) {
        pompaNyala = true;
      } else if (tanah > BATAS_POMPA_OFF) {
        pompaNyala = false;
      }
      digitalWrite(RELAY_PUMP_PIN, pompaNyala ? PUMP_LEVEL_ON : PUMP_LEVEL_OFF);

      // Logika Dynamic UV & Cahaya 
      if (uvIndex >= 6.0 || ldrPersen > BATAS_LDR_OFF) {
        lampuNyala = false;
      } else if (ldrPersen < BATAS_LDR_ON && uvIndex < 4.0) {
        lampuNyala = true;
      }
      digitalWrite(RELAY_LIGHT_PIN, lampuNyala ? LIGHT_LEVEL_ON : LIGHT_LEVEL_OFF);
    }

    // 4. TELEMETRI HTTP POST & SINKRONISASI MANUAL
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      JsonDocument doc; 
      doc["deviceId"] = "esp32-planter-01";
      doc["temperature"] = isnan(suhuUdara) ? 0.0 : suhuUdara;
      doc["humidity"] = isnan(kelembapan) ? 0.0 : kelembapan;
      doc["soilMoisture"] = tanah;
      doc["soilStatus"] = statusTanah(tanah);
      doc["soilTemperature"] = suhuTanah;
      doc["ldrLight"] = ldrPersen;
      doc["uvIndex"] = uvIndex;
      doc["pumpOn"] = pompaNyala;
      doc["growlightOn"] = lampuNyala;
      doc["mode"] = modeKontrol;

      String jsonPayload;
      serializeJson(doc, jsonPayload);

      int httpResponseCode = http.POST(jsonPayload);
      if (httpResponseCode > 0) {
        String response = http.getString();
        JsonDocument resDoc;
        if (!deserializeJson(resDoc, response)) {
          const char* modeServer = resDoc["mode"] | "AUTO";
          modeKontrol = String(modeServer);
          
          if (modeKontrol == "MANUAL") {
            pompaNyala = resDoc["pumpOn"] | false;
            lampuNyala = resDoc["growlightOn"] | false;
            digitalWrite(RELAY_PUMP_PIN, pompaNyala ? PUMP_LEVEL_ON : PUMP_LEVEL_OFF);
            digitalWrite(RELAY_LIGHT_PIN, lampuNyala ? LIGHT_LEVEL_ON : LIGHT_LEVEL_OFF);
          }
        }
        Serial.printf("Telemetri OK! HTTP: %d | Mode Server: %s\n", httpResponseCode, modeKontrol.c_str());
      } else {
        Serial.printf("Gagal mengirim data: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    } else {
      Serial.println("Wi-Fi terputus, mencoba menghubungkan ulang...");
      WiFi.reconnect();
    }

    // 5. PEMBARUAN LCD
    static bool halamanSatu = true;
    char isiLcd[17];

    if (halamanSatu) {
      if (isnan(suhuUdara) || isnan(kelembapan)) {
        tampilBaris(0, "DHT Error");
      } else {
        snprintf(isiLcd, sizeof(isiLcd), "T:%.1f%cC H:%.0f%%", suhuUdara, 223, kelembapan);
        tampilBaris(0, isiLcd);
      }
      snprintf(isiLcd, sizeof(isiLcd), "Soil:%2d%% Pmp:%s", tanah, pompaNyala ? "ON" : "OFF");
      tampilBaris(1, isiLcd);
    } else {
      snprintf(isiLcd, sizeof(isiLcd), "ST:%.1fC UV:%.1f", suhuTanah, uvIndex);
      tampilBaris(0, isiLcd);
      snprintf(isiLcd, sizeof(isiLcd), "Mode:%s L:%s", modeKontrol.c_str(), lampuNyala ? "ON" : "OFF");
      tampilBaris(1, isiLcd);
    }
    halamanSatu = !halamanSatu;

    // 6. OUTPUT SERIAL MONITOR
    Serial.printf("Soil: %d%% (%s), STanah: %.1fC -> Pompa: %s | LDR: %d%%, UV: %.2f -> Lampu: %s | T Udara: %.1fC, H: %.0f%%\n",
                  tanah, statusTanah(tanah), suhuTanah, pompaNyala ? "ON" : "OFF",
                  ldrPersen, uvIndex, lampuNyala ? "ON" : "OFF",
                  isnan(suhuUdara) ? 0.0 : suhuUdara, isnan(kelembapan) ? 0.0 : kelembapan);
  }
}