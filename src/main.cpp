// // // /**
// // //  * SMART INDOOR PLANTER - ESP32
// // //  * 
// // //  * Notes (IDE)
// // //  * Dikembangkan dan dikompilasi menggunakan Visual Studio Code (VS Code) 
// // //  * dengan ekstensi PlatformIO.
// // //  * Fitur Utama:
// // //  * 1. Otomasi Pompa Air (berdasarkan sensor kelembapan tanah dengan Hysteresis)
// // //  * 2. Otomasi Grow Light (berdasarkan sensor LDR dengan Hysteresis)
// // //  * 3. Monitoring Lokal via LCD 16x2 (I2C)
// // //  * 4. Telemetri Real-Time ke Backend Node.js via HTTP POST
// // //  */

// // // #include <Arduino.h>
// // // #include <Wire.h>
// // // #include <LiquidCrystal_I2C.h>
// // // #include <DHT.h>
// // // #include <WiFi.h>
// // // #include <HTTPClient.h>

// // // // ================= KONFIGURASI JARINGAN =================
// // // const char* ssid       = "Jojo";
// // // const char* password   = "Jojoo1234";
// // // const char* serverUrl  = "http://172.20.10.5:4000/api/telemetry"; 

// // // // ================= DEFINISI PIN =================
// // // // Sensor
// // // #define DHT_PIN         4
// // // #define DHT_TYPE        DHT11
// // // #define SOIL_PIN        34       // ADC1: Soil Moisture
// // // #define UV_PIN          35       // ADC1: Sensor UV
// // // #define LDR_PIN         32       // ADC1: Modul LDR
// // // #define SOIL_PWR        25       // VCC Soil Moisture (Hanya aktif saat membaca)

// // // // Aktuator & I2C
// // // #define RELAY_LIGHT_PIN 26       // Relay Grow Light
// // // #define RELAY_PUMP_PIN  13       // Relay Pompa Air
// // // #define I2C_SDA         21
// // // #define I2C_SCL         22
// // // #define LCD_ADDR        0x27

// // // // ================= KONFIGURASI RELAY =================
// // // // Sesuaikan dengan modul relay (true = Active-Low, false = Active-High)
// // // #define LIGHT_ACTIVE_LOW  false
// // // #define LIGHT_LEVEL_ON   (LIGHT_ACTIVE_LOW ? LOW  : HIGH)
// // // #define LIGHT_LEVEL_OFF  (LIGHT_ACTIVE_LOW ? HIGH : LOW)

// // // #define PUMP_ACTIVE_LOW   true
// // // #define PUMP_LEVEL_ON    (PUMP_ACTIVE_LOW ? LOW  : HIGH)
// // // #define PUMP_LEVEL_OFF   (PUMP_ACTIVE_LOW ? HIGH : LOW)

// // // // ================= KALIBRASI & AMBANG BATAS =================
// // // #define SOIL_DRY        0        // Nilai ADC saat di udara terbuka
// // // #define SOIL_WET        2450     // Nilai ADC saat tercelup air sepenuhnya

// // // #define BATAS_POMPA_ON   30      // Pompa ON jika tanah < 30%
// // // #define BATAS_POMPA_OFF  70      // Pompa OFF jika tanah > 70%

// // // #define BATAS_LDR_ON     60      // Lampu ON jika cahaya < 60%
// // // #define BATAS_LDR_OFF    80      // Lampu OFF jika cahaya > 80%

// // // // ================= INISIALISASI GLOBAL =================
// // // DHT dht(DHT_PIN, DHT_TYPE);
// // // LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// // // bool pompaNyala = false;
// // // bool lampuNyala = false;

// // // // ================= FUNGSI BANTUAN =================

// // // // Menampilkan teks pada baris LCD secara rata kiri
// // // void tampilBaris(uint8_t baris, const char *teks) {
// // //   char buf[17];
// // //   snprintf(buf, sizeof(buf), "%-16s", teks);
// // //   lcd.setCursor(0, baris);
// // //   lcd.print(buf);
// // // }

// // // // Membaca dan mengonversi nilai sensor tanah ke persentase (0-100%)
// // // int bacaTanahPersen() {
// // //   digitalWrite(SOIL_PWR, HIGH);     
// // //   delay(300);                       

// // //   long total = 0;
// // //   for (int i = 0; i < 10; i++) {    
// // //     total += analogRead(SOIL_PIN);
// // //     delay(10);
// // //   }
// // //   digitalWrite(SOIL_PWR, LOW);      

// // //   int raw    = total / 10;
// // //   int persen = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
// // //   return constrain(persen, 0, 100);
// // // }

// // // // Menerjemahkan persentase tanah ke status string
// // // const char *statusTanah(int persen) {
// // //   if (persen < BATAS_POMPA_ON)   return "KERING";
// // //   if (persen <= BATAS_POMPA_OFF) return "LEMBAB";
// // //   return "BASAH";
// // // }

// // // // ================= SETUP =================
// // // void setup() {
// // //   Serial.begin(115200);
// // //   delay(1000);

// // //   // 1. Setup Relay (Pastikan kondisi awal MATI)
// // //   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);
// // //   pinMode(RELAY_LIGHT_PIN, OUTPUT);
// // //   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);

// // //   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);
// // //   pinMode(RELAY_PUMP_PIN, OUTPUT);
// // //   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);

// // //   // 2. Setup Sensor Power
// // //   pinMode(SOIL_PWR, OUTPUT);
// // //   digitalWrite(SOIL_PWR, LOW);      

// // //   // 3. Setup Komunikasi & LCD
// // //   Wire.begin(I2C_SDA, I2C_SCL);
// // //   dht.begin();
// // //   lcd.init();
// // //   lcd.backlight();
  
// // //   tampilBaris(0, "Smart Planter");
// // //   tampilBaris(1, "Connecting WiFi");

// // //   // 4. Koneksi Wi-Fi
// // //   WiFi.begin(ssid, password);
// // //   Serial.printf("Menghubungkan ke Wi-Fi: %s\n", ssid);
// // //   while (WiFi.status() != WL_CONNECTED) {
// // //     delay(500);
// // //     Serial.print(".");
// // //   }
  
// // //   Serial.println("\nWi-Fi Terhubung!");
// // //   Serial.print("IP Address: ");
// // //   Serial.println(WiFi.localIP());

// // //   tampilBaris(1, "WiFi Connected");
// // //   delay(2000);
// // //   lcd.clear();
// // //   Serial.println("=== SISTEM OTOMASI SMART INDOOR PLANTER AKTIF ===");
// // // }

// // // // ================= LOOP UTAMA =================
// // // void loop() {
// // //   // 1. PEMBACAAN SENSOR
// // //   float suhu       = dht.readTemperature();
// // //   float kelembapan = dht.readHumidity();
// // //   int   tanah      = bacaTanahPersen();
  
// // //   int   ldrRaw     = analogRead(LDR_PIN);
// // //   int   ldrPersen  = constrain(map(ldrRaw, 4095, 0, 0, 100), 0, 100);
  
// // //   int   uvRaw      = analogRead(UV_PIN);
// // //   float uvIndex    = ((uvRaw * 3.3) / 4095.0) / 0.1; 

// // //   // 2. LOGIKA KONTROL POMPA (Hysteresis)
// // //   if (tanah < BATAS_POMPA_ON) {
// // //     pompaNyala = true;
// // //   } else if (tanah > BATAS_POMPA_OFF) {
// // //     pompaNyala = false;
// // //   }
// // //   digitalWrite(RELAY_PUMP_PIN, pompaNyala ? PUMP_LEVEL_ON : PUMP_LEVEL_OFF);

// // //   // 3. LOGIKA KONTROL CAHAYA (Hysteresis)
// // //   if (ldrPersen < BATAS_LDR_ON) {
// // //     lampuNyala = true;
// // //   } else if (ldrPersen > BATAS_LDR_OFF) {
// // //     lampuNyala = false;
// // //   }
// // //   digitalWrite(RELAY_LIGHT_PIN, lampuNyala ? LIGHT_LEVEL_ON : LIGHT_LEVEL_OFF);

// // //   // 4. TELEMETRI HTTP POST
// // //   if (WiFi.status() == WL_CONNECTED) {
// // //     HTTPClient http;
// // //     http.begin(serverUrl);
// // //     http.addHeader("Content-Type", "application/json");

// // //     // Pembentukan payload JSON
// // //     String jsonPayload = "{";
// // //     jsonPayload += "\"deviceId\":\"esp32-planter-01\",";
// // //     jsonPayload += "\"temperature\":" + String(isnan(suhu) ? 0.0 : suhu) + ",";
// // //     jsonPayload += "\"humidity\":" + String(isnan(kelembapan) ? 0.0 : kelembapan) + ",";
// // //     jsonPayload += "\"soilMoisture\":" + String(tanah) + ",";
// // //     jsonPayload += "\"soilStatus\":\"" + String(statusTanah(tanah)) + "\",";
// // //     jsonPayload += "\"ldrLight\":" + String(ldrPersen) + ",";
// // //     jsonPayload += "\"uvIndex\":" + String(uvIndex) + ",";
// // //     jsonPayload += "\"pumpOn\":" + String(pompaNyala ? "true" : "false") + ",";
// // //     jsonPayload += "\"growlightOn\":" + String(lampuNyala ? "true" : "false");
// // //     jsonPayload += "}";

// // //     int httpResponseCode = http.POST(jsonPayload);
// // //     if (httpResponseCode > 0) {
// // //       Serial.printf("Telemetri terkirim! HTTP Response: %d\n", httpResponseCode);
// // //     } else {
// // //       Serial.printf("Gagal mengirim data: %s\n", http.errorToString(httpResponseCode).c_str());
// // //     }
// // //     http.end();
// // //   } else {
// // //     Serial.println("Wi-Fi terputus, mencoba menghubungkan ulang...");
// // //     WiFi.reconnect();
// // //   }

// // //   // 5. PEMBARUAN LCD (Bergantian setiap iterasi loop)
// // //   static bool halamanSatu = true;
// // //   char isiLcd[17];

// // //   if (halamanSatu) {
// // //     if (isnan(suhu) || isnan(kelembapan)) {
// // //       tampilBaris(0, "DHT Error");
// // //     } else {
// // //       snprintf(isiLcd, sizeof(isiLcd), "T:%.1f%cC H:%.0f%%", suhu, 223, kelembapan);
// // //       tampilBaris(0, isiLcd);
// // //     }
// // //     snprintf(isiLcd, sizeof(isiLcd), "Soil:%2d%% Pmp:%s", tanah, pompaNyala ? "ON" : "OFF");
// // //     tampilBaris(1, isiLcd);
// // //   } else {
// // //     snprintf(isiLcd, sizeof(isiLcd), "UV:%.2f LDR:%d%%", uvIndex, ldrPersen);
// // //     tampilBaris(0, isiLcd);
// // //     snprintf(isiLcd, sizeof(isiLcd), "GrowLight : %s", lampuNyala ? "ON" : "OFF");
// // //     tampilBaris(1, isiLcd);
// // //   }
// // //   halamanSatu = !halamanSatu;

// // //   // 6. OUTPUT SERIAL MONITOR
// // //   Serial.printf("Soil: %d%% (%s) -> Pompa: %s | LDR: %d%%, UV: %.2f -> Lampu: %s | T: %.1fC, H: %.0f%%\n",
// // //                 tanah, statusTanah(tanah), pompaNyala ? "ON" : "OFF",
// // //                 ldrPersen, uvIndex, lampuNyala ? "ON" : "OFF",
// // //                 isnan(suhu) ? 0.0 : suhu, isnan(kelembapan) ? 0.0 : kelembapan);

// // //   delay(2000);
// // // }

// // /**
// //  * SMART INDOOR PLANTER - ESP32
// //  * 
// //  * Catatan Pengembangan (IDE):
// //  * Dikembangkan dan dikompilasi menggunakan Visual Studio Code (VS Code) 
// //  * dengan ekstensi PlatformIO.
// //  * 
// //  * Fitur Utama:
// //  * 1. Otomasi Pompa Air (berdasarkan sensor kelembapan tanah dengan Hysteresis)
// //  * 2. Otomasi Grow Light (berdasarkan sensor LDR dengan Hysteresis)
// //  * 3. Monitoring Lokal via LCD 16x2 (I2C)
// //  * 4. Telemetri Real-Time ke Backend Node.js via HTTP POST
// //  * 5. (Baru) Sensor Suhu Tanah DS18B20 untuk Dataset Machine Learning
// //  */

// // #include <Arduino.h>
// // #include <Wire.h>
// // #include <LiquidCrystal_I2C.h>
// // #include <DHT.h>
// // #include <WiFi.h>
// // #include <HTTPClient.h>
// // #include <OneWire.h>
// // #include <DallasTemperature.h>

// // // ================= KONFIGURASI JARINGAN =================
// // const char* ssid       = "Jojo";
// // const char* password   = "Jojoo1234";
// // const char* serverUrl  = "http://172.20.10.5:4000/api/telemetry"; 

// // // // ================= KONFIGURASI JARINGAN =================
// // // const char* ssid       = "Rukita Urbanest 2,4G";
// // // const char* password   = "hasslefree66";
// // // const char* serverUrl  = "http://192.168.1.62:4000/api/telemetry";

// // // ================= KONFIGURASI JARINGAN =================
// // // const char* ssid       = "TP-Link_96F2";
// // // const char* password   = "89508367";
// // // const char* serverUrl  = "http://192.168.0.101:4000/api/telemetry";

// // // ================= DEFINISI PIN =================
// // // Sensor
// // #define DHT_PIN         4
// // #define DHT_TYPE        DHT11
// // #define SOIL_PIN        34       // ADC1: Soil Moisture
// // #define UV_PIN          35       // ADC1: Sensor UV
// // #define LDR_PIN         32       // ADC1: Modul LDR
// // #define SOIL_PWR        25       // VCC Soil Moisture (Hanya aktif saat membaca)
// // #define DS18B20_PIN     27       // Digital 1-Wire: Sensor Suhu Tanah DS18B20

// // // Aktuator & I2C
// // #define RELAY_LIGHT_PIN 26       // Relay Grow Light
// // #define RELAY_PUMP_PIN  13       // Relay Pompa Air
// // #define I2C_SDA         21
// // #define I2C_SCL         22
// // #define LCD_ADDR        0x27

// // // ================= KONFIGURASI RELAY =================
// // #define LIGHT_ACTIVE_LOW  false
// // #define LIGHT_LEVEL_ON   (LIGHT_ACTIVE_LOW ? LOW  : HIGH)
// // #define LIGHT_LEVEL_OFF  (LIGHT_ACTIVE_LOW ? HIGH : LOW)

// // #define PUMP_ACTIVE_LOW   true
// // #define PUMP_LEVEL_ON    (PUMP_ACTIVE_LOW ? LOW  : HIGH)
// // #define PUMP_LEVEL_OFF   (PUMP_ACTIVE_LOW ? HIGH : LOW)

// // // ================= KALIBRASI & AMBANG BATAS =================
// // #define SOIL_DRY        0        // Nilai ADC saat di udara terbuka
// // #define SOIL_WET        2450     // Nilai ADC saat tercelup air sepenuhnya

// // #define BATAS_POMPA_ON   30      // Pompa ON jika tanah < 30%
// // #define BATAS_POMPA_OFF  70      // Pompa OFF jika tanah > 70%

// // #define BATAS_LDR_ON     60      // Lampu ON jika cahaya < 60%
// // #define BATAS_LDR_OFF    80      // Lampu OFF jika cahaya > 80%

// // // ================= INISIALISASI GLOBAL =================
// // DHT dht(DHT_PIN, DHT_TYPE);
// // LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// // // Inisialisasi DS18B20 (Sensor Suhu Tanah)
// // OneWire oneWire(DS18B20_PIN);
// // DallasTemperature sensorSuhuTanah(&oneWire);

// // bool pompaNyala = false;
// // bool lampuNyala = false;

// // // ================= FUNGSI BANTUAN =================
// // void tampilBaris(uint8_t baris, const char *teks) {
// //   char buf[17];
// //   snprintf(buf, sizeof(buf), "%-16s", teks);
// //   lcd.setCursor(0, baris);
// //   lcd.print(buf);
// // }

// // int bacaTanahPersen() {
// //   digitalWrite(SOIL_PWR, HIGH);     
// //   delay(300);                       

// //   long total = 0;
// //   for (int i = 0; i < 10; i++) {    
// //     total += analogRead(SOIL_PIN);
// //     delay(10);
// //   }
// //   digitalWrite(SOIL_PWR, LOW);      

// //   int raw    = total / 10;
// //   int persen = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
// //   return constrain(persen, 0, 100);
// // }

// // const char *statusTanah(int persen) {
// //   if (persen < BATAS_POMPA_ON)   return "KERING";
// //   if (persen <= BATAS_POMPA_OFF) return "LEMBAB";
// //   return "BASAH";
// // }

// // // ================= SETUP =================
// // void setup() {
// //   Serial.begin(115200);
// //   delay(1000);

// //   // 1. Setup Relay
// //   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);
// //   pinMode(RELAY_LIGHT_PIN, OUTPUT);
// //   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);

// //   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);
// //   pinMode(RELAY_PUMP_PIN, OUTPUT);
// //   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);

// //   // 2. Setup Sensor Power & Komunikasi
// //   pinMode(SOIL_PWR, OUTPUT);
// //   digitalWrite(SOIL_PWR, LOW);      

// //   Wire.begin(I2C_SDA, I2C_SCL);
// //   dht.begin();
// //   sensorSuhuTanah.begin(); // Memulai komunikasi dengan DS18B20
  
// //   // 3. Setup LCD
// //   lcd.init();
// //   lcd.backlight();
// //   tampilBaris(0, "Smart Planter");
// //   tampilBaris(1, "Connecting WiFi");

// //   // 4. Koneksi Wi-Fi
// //   WiFi.begin(ssid, password);
// //   Serial.printf("Menghubungkan ke Wi-Fi: %s\n", ssid);
// //   while (WiFi.status() != WL_CONNECTED) {
// //     delay(500);
// //     Serial.print(".");
// //   }
  
// //   Serial.println("\nWi-Fi Terhubung!");
// //   Serial.print("IP Address: ");
// //   Serial.println(WiFi.localIP());

// //   tampilBaris(1, "WiFi Connected");
// //   delay(2000);
// //   lcd.clear();
// //   Serial.println("=== SISTEM OTOMASI SMART INDOOR PLANTER AKTIF ===");
// // }

// // // ================= LOOP UTAMA =================
// // void loop() {
// //   // 1. PEMBACAAN SENSOR
// //   float suhuUdara  = dht.readTemperature();
// //   float kelembapan = dht.readHumidity();
// //   int   tanah      = bacaTanahPersen();
  
// //   // Membaca suhu tanah dari DS18B20
// //   sensorSuhuTanah.requestTemperatures(); 
// //   float suhuTanah = sensorSuhuTanah.getTempCByIndex(0);
  
// //   int   ldrRaw     = analogRead(LDR_PIN);
// //   int   ldrPersen  = constrain(map(ldrRaw, 4095, 0, 0, 100), 0, 100);
  
// //   int   uvRaw      = analogRead(UV_PIN);
// //   float uvIndex    = ((uvRaw * 3.3) / 4095.0) / 0.1; 

// //   // 2. LOGIKA KONTROL POMPA
// //   if (tanah < BATAS_POMPA_ON) {
// //     pompaNyala = true;
// //   } else if (tanah > BATAS_POMPA_OFF) {
// //     pompaNyala = false;
// //   }
// //   digitalWrite(RELAY_PUMP_PIN, pompaNyala ? PUMP_LEVEL_ON : PUMP_LEVEL_OFF);

// //   // 3. LOGIKA KONTROL CAHAYA
// //   if (ldrPersen < BATAS_LDR_ON) {
// //     lampuNyala = true;
// //   } else if (ldrPersen > BATAS_LDR_OFF) {
// //     lampuNyala = false;
// //   }
// //   digitalWrite(RELAY_LIGHT_PIN, lampuNyala ? LIGHT_LEVEL_ON : LIGHT_LEVEL_OFF);

// //   // 4. TELEMETRI HTTP POST
// //   if (WiFi.status() == WL_CONNECTED) {
// //     HTTPClient http;
// //     http.begin(serverUrl);
// //     http.addHeader("Content-Type", "application/json");

// //     // Pembentukan payload JSON (Ditambahkan atribut "soilTemperature")
// //     String jsonPayload = "{";
// //     jsonPayload += "\"deviceId\":\"esp32-planter-01\",";
// //     jsonPayload += "\"temperature\":" + String(isnan(suhuUdara) ? 0.0 : suhuUdara) + ",";
// //     jsonPayload += "\"humidity\":" + String(isnan(kelembapan) ? 0.0 : kelembapan) + ",";
// //     jsonPayload += "\"soilMoisture\":" + String(tanah) + ",";
// //     jsonPayload += "\"soilStatus\":\"" + String(statusTanah(tanah)) + "\",";
// //     jsonPayload += "\"soilTemperature\":" + String(suhuTanah == DEVICE_DISCONNECTED_C ? 0.0 : suhuTanah) + ",";
// //     jsonPayload += "\"ldrLight\":" + String(ldrPersen) + ",";
// //     jsonPayload += "\"uvIndex\":" + String(uvIndex) + ",";
// //     jsonPayload += "\"pumpOn\":" + String(pompaNyala ? "true" : "false") + ",";
// //     jsonPayload += "\"growlightOn\":" + String(lampuNyala ? "true" : "false");
// //     jsonPayload += "}";

// //     int httpResponseCode = http.POST(jsonPayload);
// //     if (httpResponseCode > 0) {
// //       Serial.printf("Telemetri terkirim! HTTP Response: %d\n", httpResponseCode);
// //     } else {
// //       Serial.printf("Gagal mengirim data: %s\n", http.errorToString(httpResponseCode).c_str());
// //     }
// //     http.end();
// //   } else {
// //     Serial.println("Wi-Fi terputus, mencoba menghubungkan ulang...");
// //     WiFi.reconnect();
// //   }

// //   // 5. PEMBARUAN LCD
// //   static bool halamanSatu = true;
// //   char isiLcd[17];

// //   if (halamanSatu) {
// //     if (isnan(suhuUdara) || isnan(kelembapan)) {
// //       tampilBaris(0, "DHT Error");
// //     } else {
// //       // Mengubah variabel ke suhuUdara agar tidak bentrok nama
// //       snprintf(isiLcd, sizeof(isiLcd), "T:%.1f%cC H:%.0f%%", suhuUdara, 223, kelembapan);
// //       tampilBaris(0, isiLcd);
// //     }
// //     snprintf(isiLcd, sizeof(isiLcd), "Soil:%2d%% Pmp:%s", tanah, pompaNyala ? "ON" : "OFF");
// //     tampilBaris(1, isiLcd);
// //   } else {
// //     // Menambahkan Suhu Tanah di LCD halaman dua
// //     snprintf(isiLcd, sizeof(isiLcd), "ST:%.1fC UV:%.1f", suhuTanah == DEVICE_DISCONNECTED_C ? 0.0 : suhuTanah, uvIndex);
// //     tampilBaris(0, isiLcd);
// //     snprintf(isiLcd, sizeof(isiLcd), "GrowLight : %s", lampuNyala ? "ON" : "OFF");
// //     tampilBaris(1, isiLcd);
// //   }
// //   halamanSatu = !halamanSatu;

// //   // 6. OUTPUT SERIAL MONITOR
// //   Serial.printf("Soil: %d%% (%s), Suhu Tanah: %.1fC -> Pompa: %s | LDR: %d%%, UV: %.2f -> Lampu: %s | T Udara: %.1fC, H: %.0f%%\n",
// //                 tanah, statusTanah(tanah), suhuTanah == DEVICE_DISCONNECTED_C ? 0.0 : suhuTanah, pompaNyala ? "ON" : "OFF",
// //                 ldrPersen, uvIndex, lampuNyala ? "ON" : "OFF",
// //                 isnan(suhuUdara) ? 0.0 : suhuUdara, isnan(kelembapan) ? 0.0 : kelembapan);

// //   delay(2000);
// // }

// /**
//  * SMART INDOOR PLANTER - ESP32 (BEST PRACTICE VERSION)
//  * 
//  * Catatan Pengembangan (IDE):
//  * Dikembangkan dan dikompilasi menggunakan Visual Studio Code (VS Code) 
//  * dengan ekstensi PlatformIO.
//  * 
//  * Pembaruan Best Practice:
//  * - Menggunakan ArduinoJson untuk manajemen memori payload yang efisien.
//  * - Menggunakan millis() pada loop utama untuk eksekusi non-blocking.
//  */

// #include <Arduino.h>
// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>
// #include <DHT.h>
// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <ArduinoJson.h>
// #include <OneWire.h>
// #include <DallasTemperature.h>

// // ================= KONFIGURASI JARINGAN =================
// const char* ssid       = "TP-Link_96F2";
// const char* password   = "89508367";
// const char* serverUrl  = "http://192.168.0.100:4000/api/telemetry";

// // ================= DEFINISI PIN =================
// #define DHT_PIN         4
// #define DHT_TYPE        DHT11
// #define SOIL_PIN        34
// #define UV_PIN          35
// #define LDR_PIN         32
// #define SOIL_PWR        25
// #define DS18B20_PIN     27

// // ================= KONFIGURASI AKTUATOR & I2C =================
// #define RELAY_LIGHT_PIN 26
// #define RELAY_PUMP_PIN  13
// #define I2C_SDA         21
// #define I2C_SCL         22
// #define LCD_ADDR        0x27

// #define LIGHT_ACTIVE_LOW  false
// #define LIGHT_LEVEL_ON   (LIGHT_ACTIVE_LOW ? LOW  : HIGH)
// #define LIGHT_LEVEL_OFF  (LIGHT_ACTIVE_LOW ? HIGH : LOW)

// #define PUMP_ACTIVE_LOW   true
// #define PUMP_LEVEL_ON    (PUMP_ACTIVE_LOW ? LOW  : HIGH)
// #define PUMP_LEVEL_OFF   (PUMP_ACTIVE_LOW ? HIGH : LOW)

// // ================= KALIBRASI & AMBANG BATAS =================
// #define SOIL_DRY        0
// #define SOIL_WET        2450
// #define BATAS_POMPA_ON   30
// #define BATAS_POMPA_OFF  70
// #define BATAS_LDR_ON     60
// #define BATAS_LDR_OFF    80

// // ================= INISIALISASI GLOBAL =================
// DHT dht(DHT_PIN, DHT_TYPE);
// LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
// OneWire oneWire(DS18B20_PIN);
// DallasTemperature sensorSuhuTanah(&oneWire);

// bool pompaNyala = false;
// bool lampuNyala = false;

// // Variabel untuk Non-Blocking Timer (millis)
// unsigned long waktuSebelumnya = 0;
// const long intervalLoop = 2000; // Eksekusi setiap 2000 ms (2 detik)

// // ================= FUNGSI BANTUAN =================
// void tampilBaris(uint8_t baris, const char *teks) {
//   char buf[17];
//   snprintf(buf, sizeof(buf), "%-16s", teks);
//   lcd.setCursor(0, baris);
//   lcd.print(buf);
// }

// int bacaTanahPersen() {
//   digitalWrite(SOIL_PWR, HIGH);     
//   delay(300); // Delay kecil saat baca sensor masih ditoleransi                       
//   long total = 0;
//   for (int i = 0; i < 10; i++) {    
//     total += analogRead(SOIL_PIN);
//     delay(10);
//   }
//   digitalWrite(SOIL_PWR, LOW);      
//   int raw    = total / 10;
//   int persen = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
//   return constrain(persen, 0, 100);
// }

// const char *statusTanah(int persen) {
//   if (persen < BATAS_POMPA_ON)   return "KERING";
//   if (persen <= BATAS_POMPA_OFF) return "LEMBAB";
//   return "BASAH";
// }

// // ================= SETUP =================
// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);
//   pinMode(RELAY_LIGHT_PIN, OUTPUT);
//   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);

//   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);
//   pinMode(RELAY_PUMP_PIN, OUTPUT);
//   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);

//   pinMode(SOIL_PWR, OUTPUT);
//   digitalWrite(SOIL_PWR, LOW);      

//   Wire.begin(I2C_SDA, I2C_SCL);
//   dht.begin();
//   sensorSuhuTanah.begin();
  
//   lcd.init();
//   lcd.backlight();
//   tampilBaris(0, "Smart Planter");
//   tampilBaris(1, "Connecting WiFi");

//   WiFi.begin(ssid, password);
//   Serial.printf("Menghubungkan ke Wi-Fi: %s\n", ssid);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
  
//   Serial.println("\nWi-Fi Terhubung!");
//   Serial.print("IP Address: ");
//   Serial.println(WiFi.localIP());

//   tampilBaris(1, "WiFi Connected");
//   delay(2000);
//   lcd.clear();
//   Serial.println("=== SISTEM OTOMASI SMART INDOOR PLANTER AKTIF ===");
// }

// // ================= LOOP UTAMA =================
// void loop() {
//   unsigned long waktuSekarang = millis();

//   // Jalankan blok kode ini hanya jika sudah berlalu 2000ms (2 detik)
//   if (waktuSekarang - waktuSebelumnya >= intervalLoop) {
//     waktuSebelumnya = waktuSekarang; // Reset timer

//     // 1. PEMBACAAN SENSOR
//     float suhuUdara  = dht.readTemperature();
//     float kelembapan = dht.readHumidity();
//     int   tanah      = bacaTanahPersen();
    
//     sensorSuhuTanah.requestTemperatures(); 
//     float suhuTanah = sensorSuhuTanah.getTempCByIndex(0);
    
//     int   ldrRaw     = analogRead(LDR_PIN);
//     int   ldrPersen  = constrain(map(ldrRaw, 4095, 0, 0, 100), 0, 100);
    
//     int   uvRaw      = analogRead(UV_PIN);
//     float uvIndex    = ((uvRaw * 3.3) / 4095.0) / 0.1; 

//     // 2. LOGIKA KONTROL POMPA
//     if (tanah < BATAS_POMPA_ON) {
//       pompaNyala = true;
//     } else if (tanah > BATAS_POMPA_OFF) {
//       pompaNyala = false;
//     }
//     digitalWrite(RELAY_PUMP_PIN, pompaNyala ? PUMP_LEVEL_ON : PUMP_LEVEL_OFF);

//     // 3. LOGIKA KONTROL CAHAYA
//     if (ldrPersen < BATAS_LDR_ON) {
//       lampuNyala = true;
//     } else if (ldrPersen > BATAS_LDR_OFF) {
//       lampuNyala = false;
//     }
//     digitalWrite(RELAY_LIGHT_PIN, lampuNyala ? LIGHT_LEVEL_ON : LIGHT_LEVEL_OFF);

//     // 4. TELEMETRI HTTP POST (Menggunakan ArduinoJson - BEST PRACTICE)
//     if (WiFi.status() == WL_CONNECTED) {
//       HTTPClient http;
//       http.begin(serverUrl);
//       http.addHeader("Content-Type", "application/json");

//       // Membuat objek JSON yang aman untuk memori
//       JsonDocument doc; 
//       doc["deviceId"] = "esp32-planter-01";
//       doc["temperature"] = isnan(suhuUdara) ? 0.0 : suhuUdara;
//       doc["humidity"] = isnan(kelembapan) ? 0.0 : kelembapan;
//       doc["soilMoisture"] = tanah;
//       doc["soilStatus"] = statusTanah(tanah);
//       doc["soilTemperature"] = (suhuTanah == DEVICE_DISCONNECTED_C) ? 0.0 : suhuTanah;
//       doc["ldrLight"] = ldrPersen;
//       doc["uvIndex"] = uvIndex;
//       doc["pumpOn"] = pompaNyala;
//       doc["growlightOn"] = lampuNyala;

//       // Konversi objek JSON menjadi String untuk dikirim
//       String jsonPayload;
//       serializeJson(doc, jsonPayload);

//       int httpResponseCode = http.POST(jsonPayload);
//       if (httpResponseCode > 0) {
//         Serial.printf("Telemetri terkirim! HTTP Response: %d\n", httpResponseCode);
//       } else {
//         Serial.printf("Gagal mengirim data: %s\n", http.errorToString(httpResponseCode).c_str());
//       }
//       http.end();
//     } else {
//       Serial.println("Wi-Fi terputus, mencoba menghubungkan ulang...");
//       WiFi.reconnect();
//     }

//     // 5. PEMBARUAN LCD
//     static bool halamanSatu = true;
//     char isiLcd[17];

//     if (halamanSatu) {
//       if (isnan(suhuUdara) || isnan(kelembapan)) {
//         tampilBaris(0, "DHT Error");
//       } else {
//         snprintf(isiLcd, sizeof(isiLcd), "T:%.1f%cC H:%.0f%%", suhuUdara, 223, kelembapan);
//         tampilBaris(0, isiLcd);
//       }
//       snprintf(isiLcd, sizeof(isiLcd), "Soil:%2d%% Pmp:%s", tanah, pompaNyala ? "ON" : "OFF");
//       tampilBaris(1, isiLcd);
//     } else {
//       snprintf(isiLcd, sizeof(isiLcd), "ST:%.1fC UV:%.1f", (suhuTanah == DEVICE_DISCONNECTED_C) ? 0.0 : suhuTanah, uvIndex);
//       tampilBaris(0, isiLcd);
//       snprintf(isiLcd, sizeof(isiLcd), "GrowLight : %s", lampuNyala ? "ON" : "OFF");
//       tampilBaris(1, isiLcd);
//     }
//     halamanSatu = !halamanSatu;

//     // 6. OUTPUT SERIAL MONITOR
//     Serial.printf("Soil: %d%% (%s), Suhu Tanah: %.1fC -> Pompa: %s | LDR: %d%%, UV: %.2f -> Lampu: %s | T Udara: %.1fC, H: %.0f%%\n",
//                   tanah, statusTanah(tanah), (suhuTanah == DEVICE_DISCONNECTED_C) ? 0.0 : suhuTanah, pompaNyala ? "ON" : "OFF",
//                   ldrPersen, uvIndex, lampuNyala ? "ON" : "OFF",
//                   isnan(suhuUdara) ? 0.0 : suhuUdara, isnan(kelembapan) ? 0.0 : kelembapan);
//   }
// }

/**
 * SMART INDOOR PLANTER - ESP32 (BEST PRACTICE VERSION)
 * 
 * Catatan Pengembangan (IDE):
 * Dikembangkan dan dikompilasi menggunakan Visual Studio Code (VS Code) 
 * dengan ekstensi PlatformIO.
 * 
 * Pembaruan:
 * - Filter Anomali 85.0C pada sensor DS18B20
 * - Logika Dynamic UV Index untuk Grow Light
 * - Sinkronisasi Kontrol Dua Arah (AUTO/MANUAL) via HTTP Response
 */

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
// ================= KONFIGURASI JARINGAN =================
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