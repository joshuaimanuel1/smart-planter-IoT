// // // // // /*
// // // // //  * Smart Indoor Planter - Rule-Based Automation & IoT Telemetry (ESP32)
// // // // //  * 1. Pompa Air (Pin 13): ON jika Soil < 30%, OFF jika Soil > 70% (Hysteresis)
// // // // //  * 2. Grow Light (Pin 26): ON jika LDR < 60%, OFF jika LDR > 80% (Hysteresis)
// // // // //  * 3. LCD 16x2 menampilkan status sensor dan aktuator secara bergantian.
// // // // //  * 4. Kirim data telemetri secara real-time ke Backend API via HTTP POST.
// // // // //  */

// // // // // #include <Arduino.h>
// // // // // #include <Wire.h>
// // // // // #include <LiquidCrystal_I2C.h>
// // // // // #include <DHT.h>
// // // // // #include <WiFi.h>
// // // // // #include <HTTPClient.h>

// // // // // // // ---------- Konfigurasi Wi-Fi & Server Backend ----------
// // // // // // const char* ssid       = "Jojo";              // Sesuai nama hotspot HP kamu[cite: 24, 25]
// // // // // // const char* password   = "Jojoo1234";         // Password hotspot HP kamu[cite: 24]

// // // // // // // URL Backend menggunakan IPv4 laptop kamu (port 4000)[cite: 20, 25]
// // // // // // const char* serverUrl  = "http://172.20.10.5:4000/api/telemetry"; 

// // // // // // ---------- Konfigurasi Wi-Fi & Server Backend ----------
// // // // // const char* ssid       = "TP-Link_96F2";        
// // // // // const char* password   = "89508367";            

// // // // // // URL Backend menggunakan IPv4 laptop di jaringan TP-Link (port 4000)
// // // // // const char* serverUrl  = "http://192.168.0.100:4000/api/telemetry";

// // // // // // ---------- Definisi Pin ----------
// // // // // #define DHT_PIN         4        // DATA DHT11
// // // // // #define DHT_TYPE        DHT11
// // // // // #define SOIL_PIN        34       // AOUT Soil Moisture (ADC1)
// // // // // #define UV_PIN          35       // AOUT Sensor UV GUVA-S12SD (ADC1)
// // // // // #define LDR_PIN         32       // AOUT Modul LDR (ADC1)
// // // // // #define SOIL_PWR        25       // VCC Soil Moisture (aktif saat baca)
// // // // // #define RELAY_LIGHT_PIN 26       // IN Relay 1 (Grow Light)
// // // // // #define RELAY_PUMP_PIN  13       // IN Relay 2 (Pompa Air / Dinamo)

// // // // // #define I2C_SDA         21
// // // // // #define I2C_SCL         22
// // // // // #define LCD_ADDR        0x27

// // // // // // ---------- Konfigurasi Relay Grow Light (Pin 26) ----------
// // // // // #define LIGHT_ACTIVE_LOW  false
// // // // // #define LIGHT_LEVEL_ON   (LIGHT_ACTIVE_LOW ? LOW  : HIGH)
// // // // // #define LIGHT_LEVEL_OFF  (LIGHT_ACTIVE_LOW ? HIGH : LOW)

// // // // // // ---------- Konfigurasi Relay Pompa Air (Pin 13) ----------
// // // // // #define PUMP_ACTIVE_LOW   false
// // // // // #define PUMP_LEVEL_ON    (PUMP_ACTIVE_LOW ? LOW  : HIGH)
// // // // // #define PUMP_LEVEL_OFF   (PUMP_ACTIVE_LOW ? HIGH : LOW)

// // // // // // ---------- Kalibrasi Soil Moisture ----------
// // // // // #define SOIL_DRY        0        // Nilai analog di udara terbuka
// // // // // #define SOIL_WET        2450     // Nilai analog saat tercelup air

// // // // // // ---------- Ambang Batas (Threshold) Otomasi & Hysteresis ----------
// // // // // #define BATAS_POMPA_ON   30      // Pompa nyala jika tanah < 30%
// // // // // #define BATAS_POMPA_OFF  70      // Pompa mati jika tanah > 70%

// // // // // #define BATAS_LDR_ON     60      // Lampu nyala jika cahaya LDR < 60% (Gelap)
// // // // // #define BATAS_LDR_OFF    80      // Lampu mati jika cahaya LDR > 80% (Terang)

// // // // // DHT dht(DHT_PIN, DHT_TYPE);
// // // // // LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// // // // // // Variabel penyimpan status aktuator
// // // // // bool pompaNyala = false;
// // // // // bool lampuNyala = false;

// // // // // // ---------------------------------------------------------------
// // // // // // Fungsi Pembantu Tampilan & Sensor
// // // // // // ---------------------------------------------------------------
// // // // // void tampilBaris(uint8_t baris, const char *teks)
// // // // // {
// // // // //   char buf[17];
// // // // //   snprintf(buf, sizeof(buf), "%-16s", teks);
// // // // //   lcd.setCursor(0, baris);
// // // // //   lcd.print(buf);
// // // // // }

// // // // // int bacaTanahPersen()
// // // // // {
// // // // //   digitalWrite(SOIL_PWR, HIGH);     
// // // // //   delay(300);                       

// // // // //   long total = 0;
// // // // //   for (int i = 0; i < 10; i++) {    
// // // // //     total += analogRead(SOIL_PIN);
// // // // //     delay(10);
// // // // //   }

// // // // //   digitalWrite(SOIL_PWR, LOW);      

// // // // //   int raw    = total / 10;
// // // // //   int persen = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
// // // // //   return constrain(persen, 0, 100);
// // // // // }

// // // // // const char *statusTanah(int persen)
// // // // // {
// // // // //   if (persen < BATAS_POMPA_ON)   return "KERING";
// // // // //   if (persen <= BATAS_POMPA_OFF) return "LEMBAB";
// // // // //   return "BASAH";
// // // // // }

// // // // // // ---------------------------------------------------------------
// // // // // void setup()
// // // // // {
// // // // //   Serial.begin(115200);
// // // // //   delay(1000);

// // // // //   // Pastikan kedua relay MATI saat pertama kali menyala
// // // // //   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);
// // // // //   pinMode(RELAY_LIGHT_PIN, OUTPUT);
// // // // //   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);

// // // // //   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);
// // // // //   pinMode(RELAY_PUMP_PIN, OUTPUT);
// // // // //   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);

// // // // //   pinMode(SOIL_PWR, OUTPUT);
// // // // //   digitalWrite(SOIL_PWR, LOW);      

// // // // //   Wire.begin(I2C_SDA, I2C_SCL);
// // // // //   dht.begin();

// // // // //   lcd.init();
// // // // //   lcd.backlight();
// // // // //   tampilBaris(0, "Smart Planter");
// // // // //   tampilBaris(1, "Connecting WiFi");

// // // // //   // Koneksi Wi-Fi
// // // // //   WiFi.begin(ssid, password);
// // // // //   Serial.printf("Menghubungkan ke Wi-Fi: %s\n", ssid);
// // // // //   while (WiFi.status() != WL_CONNECTED) {
// // // // //     delay(500);
// // // // //     Serial.print(".");
// // // // //   }
// // // // //   Serial.println("\nWi-Fi Terhubung!");
// // // // //   Serial.print("IP Address ESP32: ");
// // // // //   Serial.println(WiFi.localIP());

// // // // //   tampilBaris(1, "WiFi Connected");
// // // // //   delay(2000);
// // // // //   lcd.clear();

// // // // //   Serial.println("=== SISTEM OTOMASI SMART INDOOR PLANTER AKTIF ===");
// // // // // }

// // // // // void loop()
// // // // // {
// // // // //   // 1. BACA SEMUA SENSOR
// // // // //   float suhu       = dht.readTemperature();
// // // // //   float kelembapan = dht.readHumidity();
// // // // //   int   tanah      = bacaTanahPersen();
  
// // // // //   // Baca LDR (0% = Gelap, 100% = Terang)
// // // // //   int   ldrRaw     = analogRead(LDR_PIN);
// // // // //   int   ldrPersen  = map(ldrRaw, 4095, 0, 0, 100);
// // // // //   ldrPersen        = constrain(ldrPersen, 0, 100);
  
// // // // //   // Baca UV Index (Pemantauan)
// // // // //   int   uvRaw      = analogRead(UV_PIN);
// // // // //   float uvVoltage  = (uvRaw * 3.3) / 4095.0;
// // // // //   float uvIndex    = uvVoltage / 0.1; 

// // // // //   // 2. LOGIKA OTOMASI POMPA AIR (Hysteresis: ON < 30%, OFF > 70%)
// // // // //   if (tanah < BATAS_POMPA_ON) {
// // // // //     pompaNyala = true;  // Tanah kering (< 30%), nyalakan pompa
// // // // //   } else if (tanah > BATAS_POMPA_OFF) {
// // // // //     pompaNyala = false; // Tanah sudah basah (> 70%), matikan pompa
// // // // //   }

// // // // //   digitalWrite(RELAY_PUMP_PIN, pompaNyala ? PUMP_LEVEL_ON : PUMP_LEVEL_OFF);

// // // // //   // 3. LOGIKA OTOMASI GROW LIGHT (Hysteresis: ON < 60%, OFF > 80%)
// // // // //   if (ldrPersen < BATAS_LDR_ON) {
// // // // //     lampuNyala = true;  // Ruangan gelap (< 60%), lampu menyala
// // // // //   } else if (ldrPersen > BATAS_LDR_OFF) {
// // // // //     lampuNyala = false; // Ruangan terang (> 80%), lampu mati
// // // // //   }

// // // // //   digitalWrite(RELAY_LIGHT_PIN, lampuNyala ? LIGHT_LEVEL_ON : LIGHT_LEVEL_OFF);

// // // // //   // 4. KIRIM DATA KE BACKEND SERVER VIA HTTP POST (REAL-TIME)
// // // // //   if (WiFi.status() == WL_CONNECTED) {
// // // // //     HTTPClient http;
// // // // //     http.begin(serverUrl);
// // // // //     http.addHeader("Content-Type", "application/json");

// // // // //     // Format JSON payload yang bersih dan valid untuk backend API
// // // // //     String jsonPayload = "{";
// // // // //     jsonPayload += "\"deviceId\":\"esp32-planter-01\",";
// // // // //     jsonPayload += "\"temperature\":" + String(isnan(suhu) ? 0.0 : suhu) + ",";
// // // // //     jsonPayload += "\"humidity\":" + String(isnan(kelembapan) ? 0.0 : kelembapan) + ",";
// // // // //     jsonPayload += "\"soilMoisture\":" + String(tanah) + ",";
// // // // //     jsonPayload += "\"soilStatus\":\"" + String(statusTanah(tanah)) + "\",";
// // // // //     jsonPayload += "\"ldrLight\":" + String(ldrPersen) + ",";
// // // // //     jsonPayload += "\"uvIndex\":" + String(uvIndex) + ",";
// // // // //     jsonPayload += "\"pumpOn\":" + String(pompaNyala ? "true" : "false") + ",";
// // // // //     jsonPayload += "\"growlightOn\":" + String(lampuNyala ? "true" : "false");
// // // // //     jsonPayload += "}";

// // // // //     int httpResponseCode = http.POST(jsonPayload);
// // // // //     if (httpResponseCode > 0) {
// // // // //       Serial.printf("Telemetri terkirim! HTTP Response code: %d\n", httpResponseCode);
// // // // //     } else {
// // // // //       Serial.printf("Gagal mengirim data, error: %s\n", http.errorToString(httpResponseCode).c_str());
// // // // //     }
// // // // //     http.end();
// // // // //   } else {
// // // // //     Serial.println("Wi-Fi terputus, mencoba menghubungkan ulang...");
// // // // //     WiFi.reconnect();
// // // // //   }

// // // // //   // 5. TAMPILAN LCD (Bergantian tiap 2 detik)
// // // // //   static bool halamanSatu = true;
// // // // //   char isiLcd[17];

// // // // //   if (halamanSatu) {
// // // // //     // Halaman 1: Suhu, Kelembapan, Tanah & Status Pompa
// // // // //     if (isnan(suhu) || isnan(kelembapan)) {
// // // // //       tampilBaris(0, "DHT Error");
// // // // //     } else {
// // // // //       snprintf(isiLcd, sizeof(isiLcd), "T:%.1f%cC H:%.0f%%", suhu, 223, kelembapan);
// // // // //       tampilBaris(0, isiLcd);
// // // // //     }
// // // // //     snprintf(isiLcd, sizeof(isiLcd), "Soil:%2d%% Pmp:%s", tanah, pompaNyala ? "ON" : "OFF");
// // // // //     tampilBaris(1, isiLcd);
// // // // //   } else {
// // // // //     // Halaman 2: UV, LDR & Status Grow Light
// // // // //     snprintf(isiLcd, sizeof(isiLcd), "UV:%.2f LDR:%d%%", uvIndex, ldrPersen);
// // // // //     tampilBaris(0, isiLcd);
// // // // //     snprintf(isiLcd, sizeof(isiLcd), "GrowLight : %s", lampuNyala ? "ON" : "OFF");
// // // // //     tampilBaris(1, isiLcd);
// // // // //   }
// // // // //   halamanSatu = !halamanSatu;

// // // // //   // 6. LOG SERIAL MONITOR
// // // // //   Serial.printf("Soil: %d%% (%s) -> Pompa: %s | LDR: %d%%, UV: %.2f -> Lampu: %s | T: %.1fC, H: %.0f%%\n",
// // // // //                 tanah, statusTanah(tanah), pompaNyala ? "ON" : "OFF",
// // // // //                 ldrPersen, uvIndex, lampuNyala ? "ON" : "OFF",
// // // // //                 isnan(suhu) ? 0.0 : suhu, isnan(kelembapan) ? 0.0 : kelembapan);

// // // // //   delay(2000);
// // // // // }

// // // // /*
// // // //  * Smart Indoor Planter - Rule-Based Automation & IoT Telemetry (ESP32)
// // // //  * 1. Pompa Air (Pin 13): ON jika Soil < 30%, OFF jika Soil > 70% (Hysteresis)
// // // //  * 2. Grow Light (Pin 26): ON jika LDR < 60%, OFF jika LDR > 80% (Hysteresis)
// // // //  * 3. LCD 16x2 menampilkan status sensor dan aktuator secara bergantian.
// // // //  * 4. Kirim data telemetri secara real-time ke Backend API via HTTP POST.
// // // //  */

// // // // #include <Arduino.h>
// // // // #include <Wire.h>
// // // // #include <LiquidCrystal_I2C.h>
// // // // #include <DHT.h>
// // // // #include <WiFi.h>
// // // // #include <HTTPClient.h>

// // // // // ---------- Konfigurasi Wi-Fi & Server Backend ----------
// // // // const char* ssid       = "Jojo";              // Sesuai nama hotspot HP kamu
// // // // const char* password   = "Jojoo1234";         // Password hotspot HP kamu

// // // // // URL Backend menggunakan IPv4 laptop kamu (port 4000)
// // // // const char* serverUrl  = "http://172.20.10.5:4000/api/telemetry"; 

// // // // // ---------- Definisi Pin ----------
// // // // #define DHT_PIN         4        // DATA DHT11
// // // // #define DHT_TYPE        DHT11
// // // // #define SOIL_PIN        34       // AOUT Soil Moisture (ADC1)
// // // // #define UV_PIN          35       // AOUT Sensor UV GUVA-S12SD (ADC1)
// // // // #define LDR_PIN         32       // AOUT Modul LDR (ADC1)
// // // // #define SOIL_PWR        25       // VCC Soil Moisture (aktif saat baca)
// // // // #define RELAY_LIGHT_PIN 26       // IN Relay 1 (Grow Light)
// // // // #define RELAY_PUMP_PIN  13       // IN Relay 2 (Pompa Air / Dinamo)

// // // // #define I2C_SDA         21
// // // // #define I2C_SCL         22
// // // // #define LCD_ADDR        0x27

// // // // // ---------- Konfigurasi Relay Grow Light (Pin 26) ----------
// // // // #define LIGHT_ACTIVE_LOW  false
// // // // #define LIGHT_LEVEL_ON   (LIGHT_ACTIVE_LOW ? LOW  : HIGH)
// // // // #define LIGHT_LEVEL_OFF  (LIGHT_ACTIVE_LOW ? HIGH : LOW)

// // // // // ---------- Konfigurasi Relay Pompa Air (Pin 13) ----------
// // // // // LOGIKA NO (Normally Open): Diubah menjadi true agar LOW = Pompa Hidup, HIGH = Pompa Mati
// // // // #define PUMP_ACTIVE_LOW   true
// // // // #define PUMP_LEVEL_ON    (PUMP_ACTIVE_LOW ? LOW  : HIGH)
// // // // #define PUMP_LEVEL_OFF   (PUMP_ACTIVE_LOW ? HIGH : LOW)

// // // // // ---------- Kalibrasi Soil Moisture ----------
// // // // #define SOIL_DRY        0        // Nilai analog di udara terbuka
// // // // #define SOIL_WET        2450     // Nilai analog saat tercelup air

// // // // // ---------- Ambang Batas (Threshold) Otomasi & Hysteresis ----------
// // // // #define BATAS_POMPA_ON   30      // Pompa nyala jika tanah < 30%
// // // // #define BATAS_POMPA_OFF  70      // Pompa mati jika tanah > 70%

// // // // #define BATAS_LDR_ON     60      // Lampu nyala jika cahaya LDR < 60% (Gelap)
// // // // #define BATAS_LDR_OFF    80      // Lampu mati jika cahaya LDR > 80% (Terang)

// // // // DHT dht(DHT_PIN, DHT_TYPE);
// // // // LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// // // // // Variabel penyimpan status aktuator
// // // // bool pompaNyala = false;
// // // // bool lampuNyala = false;

// // // // // ---------------------------------------------------------------
// // // // // Fungsi Pembantu Tampilan & Sensor
// // // // // ---------------------------------------------------------------
// // // // void tampilBaris(uint8_t baris, const char *teks)
// // // // {
// // // //   char buf[17];
// // // //   snprintf(buf, sizeof(buf), "%-16s", teks);
// // // //   lcd.setCursor(0, baris);
// // // //   lcd.print(buf);
// // // // }

// // // // int bacaTanahPersen()
// // // // {
// // // //   digitalWrite(SOIL_PWR, HIGH);     
// // // //   delay(300);                       

// // // //   long total = 0;
// // // //   for (int i = 0; i < 10; i++) {    
// // // //     total += analogRead(SOIL_PIN);
// // // //     delay(10);
// // // //   }

// // // //   digitalWrite(SOIL_PWR, LOW);      

// // // //   int raw    = total / 10;
// // // //   int persen = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
// // // //   return constrain(persen, 0, 100);
// // // // }

// // // // const char *statusTanah(int persen)
// // // // {
// // // //   if (persen < BATAS_POMPA_ON)   return "KERING";
// // // //   if (persen <= BATAS_POMPA_OFF) return "LEMBAB";
// // // //   return "BASAH";
// // // // }

// // // // // ---------------------------------------------------------------
// // // // void setup()
// // // // {
// // // //   Serial.begin(115200);
// // // //   delay(1000);

// // // //   // Pastikan kedua relay MATI saat pertama kali menyala
// // // //   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);
// // // //   pinMode(RELAY_LIGHT_PIN, OUTPUT);
// // // //   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);

// // // //   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);
// // // //   pinMode(RELAY_PUMP_PIN, OUTPUT);
// // // //   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);

// // // //   pinMode(SOIL_PWR, OUTPUT);
// // // //   digitalWrite(SOIL_PWR, LOW);      

// // // //   Wire.begin(I2C_SDA, I2C_SCL);
// // // //   dht.begin();

// // // //   lcd.init();
// // // //   lcd.backlight();
// // // //   tampilBaris(0, "Smart Planter");
// // // //   tampilBaris(1, "Connecting WiFi");

// // // //   // Koneksi Wi-Fi
// // // //   WiFi.begin(ssid, password);
// // // //   Serial.printf("Menghubungkan ke Wi-Fi: %s\n", ssid);
// // // //   while (WiFi.status() != WL_CONNECTED) {
// // // //     delay(500);
// // // //     Serial.print(".");
// // // //   }
// // // //   Serial.println("\nWi-Fi Terhubung!");
// // // //   Serial.print("IP Address ESP32: ");
// // // //   Serial.println(WiFi.localIP());

// // // //   tampilBaris(1, "WiFi Connected");
// // // //   delay(2000);
// // // //   lcd.clear();

// // // //   Serial.println("=== SISTEM OTOMASI SMART INDOOR PLANTER AKTIF ===");
// // // // }

// // // // void loop()
// // // // {
// // // //   // 1. BACA SEMUA SENSOR
// // // //   float suhu       = dht.readTemperature();
// // // //   float kelembapan = dht.readHumidity();
// // // //   int   tanah      = bacaTanahPersen();
  
// // // //   // Baca LDR (0% = Gelap, 100% = Terang)
// // // //   int   ldrRaw     = analogRead(LDR_PIN);
// // // //   int   ldrPersen  = map(ldrRaw, 4095, 0, 0, 100);
// // // //   ldrPersen        = constrain(ldrPersen, 0, 100);
  
// // // //   // Baca UV Index (Pemantauan)
// // // //   int   uvRaw      = analogRead(UV_PIN);
// // // //   float uvVoltage  = (uvRaw * 3.3) / 4095.0;
// // // //   float uvIndex    = uvVoltage / 0.1; 

// // // //   // 2. LOGIKA OTOMASI POMPA AIR (Hysteresis: ON < 30%, OFF > 70%)
// // // //   if (tanah < BATAS_POMPA_ON) {
// // // //     pompaNyala = true;  // Tanah kering (< 30%), nyalakan pompa
// // // //   } else if (tanah > BATAS_POMPA_OFF) {
// // // //     pompaNyala = false; // Tanah sudah basah (> 70%), matikan pompa
// // // //   }

// // // //   digitalWrite(RELAY_PUMP_PIN, pompaNyala ? PUMP_LEVEL_ON : PUMP_LEVEL_OFF);

// // // //   // 3. LOGIKA OTOMASI GROW LIGHT (Hysteresis: ON < 60%, OFF > 80%)
// // // //   if (ldrPersen < BATAS_LDR_ON) {
// // // //     lampuNyala = true;  // Ruangan gelap (< 60%), lampu menyala
// // // //   } else if (ldrPersen > BATAS_LDR_OFF) {
// // // //     lampuNyala = false; // Ruangan terang (> 80%), lampu mati
// // // //   }

// // // //   digitalWrite(RELAY_LIGHT_PIN, lampuNyala ? LIGHT_LEVEL_ON : LIGHT_LEVEL_OFF);

// // // //   // 4. KIRIM DATA KE BACKEND SERVER VIA HTTP POST (REAL-TIME)
// // // //   if (WiFi.status() == WL_CONNECTED) {
// // // //     HTTPClient http;
// // // //     http.begin(serverUrl);
// // // //     http.addHeader("Content-Type", "application/json");

// // // //     // Format JSON payload yang bersih dan valid untuk backend API
// // // //     String jsonPayload = "{";
// // // //     jsonPayload += "\"deviceId\":\"esp32-planter-01\",";
// // // //     jsonPayload += "\"temperature\":" + String(isnan(suhu) ? 0.0 : suhu) + ",";
// // // //     jsonPayload += "\"humidity\":" + String(isnan(kelembapan) ? 0.0 : kelembapan) + ",";
// // // //     jsonPayload += "\"soilMoisture\":" + String(tanah) + ",";
// // // //     jsonPayload += "\"soilStatus\":\"" + String(statusTanah(tanah)) + "\",";
// // // //     jsonPayload += "\"ldrLight\":" + String(ldrPersen) + ",";
// // // //     jsonPayload += "\"uvIndex\":" + String(uvIndex) + ",";
// // // //     jsonPayload += "\"pumpOn\":" + String(pompaNyala ? "true" : "false") + ",";
// // // //     jsonPayload += "\"growlightOn\":" + String(lampuNyala ? "true" : "false");
// // // //     jsonPayload += "}";

// // // //     int httpResponseCode = http.POST(jsonPayload);
// // // //     if (httpResponseCode > 0) {
// // // //       Serial.printf("Telemetri terkirim! HTTP Response code: %d\n", httpResponseCode);
// // // //     } else {
// // // //       Serial.printf("Gagal mengirim data, error: %s\n", http.errorToString(httpResponseCode).c_str());
// // // //     }
// // // //     http.end();
// // // //   } else {
// // // //     Serial.println("Wi-Fi terputus, mencoba menghubungkan ulang...");
// // // //     WiFi.reconnect();
// // // //   }

// // // //   // 5. TAMPILAN LCD (Bergantian tiap 2 detik)
// // // //   static bool halamanSatu = true;
// // // //   char isiLcd[17];

// // // //   if (halamanSatu) {
// // // //     // Halaman 1: Suhu, Kelembapan, Tanah & Status Pompa
// // // //     if (isnan(suhu) || isnan(kelembapan)) {
// // // //       tampilBaris(0, "DHT Error");
// // // //     } else {
// // // //       snprintf(isiLcd, sizeof(isiLcd), "T:%.1f%cC H:%.0f%%", suhu, 223, kelembapan);
// // // //       tampilBaris(0, isiLcd);
// // // //     }
// // // //     snprintf(isiLcd, sizeof(isiLcd), "Soil:%2d%% Pmp:%s", tanah, pompaNyala ? "ON" : "OFF");
// // // //     tampilBaris(1, isiLcd);
// // // //   } else {
// // // //     // Halaman 2: UV, LDR & Status Grow Light
// // // //     snprintf(isiLcd, sizeof(isiLcd), "UV:%.2f LDR:%d%%", uvIndex, ldrPersen);
// // // //     tampilBaris(0, isiLcd);
// // // //     snprintf(isiLcd, sizeof(isiLcd), "GrowLight : %s", lampuNyala ? "ON" : "OFF");
// // // //     tampilBaris(1, isiLcd);
// // // //   }
// // // //   halamanSatu = !halamanSatu;

// // // //   // 6. LOG SERIAL MONITOR
// // // //   Serial.printf("Soil: %d%% (%s) -> Pompa: %s | LDR: %d%%, UV: %.2f -> Lampu: %s | T: %.1fC, H: %.0f%%\n",
// // // //                 tanah, statusTanah(tanah), pompaNyala ? "ON" : "OFF",
// // // //                 ldrPersen, uvIndex, lampuNyala ? "ON" : "OFF",
// // // //                 isnan(suhu) ? 0.0 : suhu, isnan(kelembapan) ? 0.0 : kelembapan);

// // // //   delay(2000);
// // // // }

// // // /*
// // //  * Smart Indoor Planter - Rule-Based Automation & IoT Telemetry (ESP32)
// // //  * 1. Pompa Air (Pin 13): ON jika Soil < 30%, OFF jika Soil > 70% (Hysteresis)
// // //  * 2. Grow Light (Pin 26): ON jika LDR < 60%, OFF jika LDR > 80% (Hysteresis)
// // //  * 3. LCD 16x2 menampilkan status sensor dan aktuator secara bergantian.
// // //  * 4. Kirim data telemetri secara real-time ke Backend API via HTTP POST.
// // //  */

// // // #include <Arduino.h>
// // // #include <Wire.h>
// // // #include <LiquidCrystal_I2C.h>
// // // #include <DHT.h>
// // // #include <WiFi.h>
// // // #include <HTTPClient.h>

// // // // ---------- Konfigurasi Wi-Fi & Server Backend ----------
// // // const char* ssid       = "Jojo";              // Sesuai nama hotspot HP kamu
// // // const char* password   = "Jojoo1234";         // Password hotspot HP kamu

// // // // URL Backend menggunakan IPv4 laptop kamu (port 4000)
// // // const char* serverUrl  = "http://172.20.10.5:4000/api/telemetry"; 

// // // // ---------- Definisi Pin ----------
// // // #define DHT_PIN         4        // DATA DHT11
// // // #define DHT_TYPE        DHT11
// // // #define SOIL_PIN        34       // AOUT Soil Moisture (ADC1)
// // // #define UV_PIN          35       // AOUT Sensor UV GUVA-S12SD (ADC1)
// // // #define LDR_PIN         32       // AOUT Modul LDR (ADC1)
// // // #define SOIL_PWR        25       // VCC Soil Moisture (aktif saat baca)
// // // #define RELAY_LIGHT_PIN 26       // IN Relay 1 (Grow Light)
// // // #define RELAY_PUMP_PIN  13       // IN Relay 2 (Pompa Air / Dinamo)

// // // #define I2C_SDA         21
// // // #define I2C_SCL         22
// // // #define LCD_ADDR        0x27

// // // // ---------- Konfigurasi Relay Grow Light (Pin 26) ----------
// // // #define LIGHT_ACTIVE_LOW  false
// // // #define LIGHT_LEVEL_ON   (LIGHT_ACTIVE_LOW ? LOW  : HIGH)
// // // #define LIGHT_LEVEL_OFF  (LIGHT_ACTIVE_LOW ? HIGH : LOW)

// // // // ---------- Konfigurasi Relay Pompa Air (Pin 13) ----------
// // // // LOGIKA NO (Normally Open): Diubah menjadi false (Active-High) agar pompa mati dengan benar
// // // #define PUMP_ACTIVE_LOW  false
// // // #define PUMP_LEVEL_ON    (PUMP_ACTIVE_LOW ? LOW  : HIGH)
// // // #define PUMP_LEVEL_OFF   (PUMP_ACTIVE_LOW ? HIGH : LOW)

// // // // ---------- Kalibrasi Soil Moisture ----------
// // // #define SOIL_DRY        0        // Nilai analog di udara terbuka
// // // #define SOIL_WET        2450     // Nilai analog saat tercelup air

// // // // ---------- Ambang Batas (Threshold) Otomasi & Hysteresis ----------
// // // #define BATAS_POMPA_ON   30      // Pompa nyala jika tanah < 30%
// // // #define BATAS_POMPA_OFF  70      // Pompa mati jika tanah > 70%

// // // #define BATAS_LDR_ON     60      // Lampu nyala jika cahaya LDR < 60% (Gelap)
// // // #define BATAS_LDR_OFF    80      // Lampu mati jika cahaya LDR > 80% (Terang)

// // // DHT dht(DHT_PIN, DHT_TYPE);
// // // LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// // // // Variabel penyimpan status aktuator
// // // bool pompaNyala = false;
// // // bool lampuNyala = false;

// // // // ---------------------------------------------------------------
// // // // Fungsi Pembantu Tampilan & Sensor
// // // // ---------------------------------------------------------------
// // // void tampilBaris(uint8_t baris, const char *teks)
// // // {
// // //   char buf[17];
// // //   snprintf(buf, sizeof(buf), "%-16s", teks);
// // //   lcd.setCursor(0, baris);
// // //   lcd.print(buf);
// // // }

// // // int bacaTanahPersen()
// // // {
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

// // // const char *statusTanah(int persen)
// // // {
// // //   if (persen < BATAS_POMPA_ON)   return "KERING";
// // //   if (persen <= BATAS_POMPA_OFF) return "LEMBAB";
// // //   return "BASAH";
// // // }

// // // // ---------------------------------------------------------------
// // // void setup()
// // // {
// // //   Serial.begin(115200);
// // //   delay(1000);

// // //   // Pastikan kedua relay MATI saat pertama kali menyala
// // //   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);
// // //   pinMode(RELAY_LIGHT_PIN, OUTPUT);
// // //   digitalWrite(RELAY_LIGHT_PIN, LIGHT_LEVEL_OFF);

// // //   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);
// // //   pinMode(RELAY_PUMP_PIN, OUTPUT);
// // //   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);

// // //   pinMode(SOIL_PWR, OUTPUT);
// // //   digitalWrite(SOIL_PWR, LOW);      

// // //   Wire.begin(I2C_SDA, I2C_SCL);
// // //   dht.begin();

// // //   lcd.init();
// // //   lcd.backlight();
// // //   tampilBaris(0, "Smart Planter");
// // //   tampilBaris(1, "Connecting WiFi");

// // //   // Koneksi Wi-Fi
// // //   WiFi.begin(ssid, password);
// // //   Serial.printf("Menghubungkan ke Wi-Fi: %s\n", ssid);
// // //   while (WiFi.status() != WL_CONNECTED) {
// // //     delay(500);
// // //     Serial.print(".");
// // //   }
// // //   Serial.println("\nWi-Fi Terhubung!");
// // //   Serial.print("IP Address ESP32: ");
// // //   Serial.println(WiFi.localIP());

// // //   tampilBaris(1, "WiFi Connected");
// // //   delay(2000);
// // //   lcd.clear();

// // //   Serial.println("=== SISTEM OTOMASI SMART INDOOR PLANTER AKTIF ===");
// // // }

// // // void loop()
// // // {
// // //   // 1. BACA SEMUA SENSOR
// // //   float suhu       = dht.readTemperature();
// // //   float kelembapan = dht.readHumidity();
// // //   int   tanah      = bacaTanahPersen();
  
// // //   // Baca LDR (0% = Gelap, 100% = Terang)
// // //   int   ldrRaw     = analogRead(LDR_PIN);
// // //   int   ldrPersen  = map(ldrRaw, 4095, 0, 0, 100);
// // //   ldrPersen        = constrain(ldrPersen, 0, 100);
  
// // //   // Baca UV Index (Pemantauan)
// // //   int   uvRaw      = analogRead(UV_PIN);
// // //   float uvVoltage  = (uvRaw * 3.3) / 4095.0;
// // //   float uvIndex    = uvVoltage / 0.1; 

// // //   // 2. LOGIKA OTOMASI POMPA AIR (Hysteresis: ON < 30%, OFF > 70%)
// // //   if (tanah < BATAS_POMPA_ON) {
// // //     pompaNyala = true;  // Tanah kering (< 30%), nyalakan pompa
// // //   } else if (tanah > BATAS_POMPA_OFF) {
// // //     pompaNyala = false; // Tanah sudah basah (> 70%), matikan pompa
// // //   }

// // //   digitalWrite(RELAY_PUMP_PIN, pompaNyala ? PUMP_LEVEL_ON : PUMP_LEVEL_OFF);

// // //   // 3. LOGIKA OTOMASI GROW LIGHT (Hysteresis: ON < 60%, OFF > 80%)
// // //   if (ldrPersen < BATAS_LDR_ON) {
// // //     lampuNyala = true;  // Ruangan gelap (< 60%), lampu menyala
// // //   } else if (ldrPersen > BATAS_LDR_OFF) {
// // //     lampuNyala = false; // Ruangan terang (> 80%), lampu mati
// // //   }

// // //   digitalWrite(RELAY_LIGHT_PIN, lampuNyala ? LIGHT_LEVEL_ON : LIGHT_LEVEL_OFF);

// // //   // 4. KIRIM DATA KE BACKEND SERVER VIA HTTP POST (REAL-TIME)
// // //   if (WiFi.status() == WL_CONNECTED) {
// // //     HTTPClient http;
// // //     http.begin(serverUrl);
// // //     http.addHeader("Content-Type", "application/json");

// // //     // Format JSON payload yang bersih dan valid untuk backend API
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
// // //       Serial.printf("Telemetri terkirim! HTTP Response code: %d\n", httpResponseCode);
// // //     } else {
// // //       Serial.printf("Gagal mengirim data, error: %s\n", http.errorToString(httpResponseCode).c_str());
// // //     }
// // //     http.end();
// // //   } else {
// // //     Serial.println("Wi-Fi terputus, mencoba menghubungkan ulang...");
// // //     WiFi.reconnect();
// // //   }

// // //   // 5. TAMPILAN LCD (Bergantian tiap 2 detik)
// // //   static bool halamanSatu = true;
// // //   char isiLcd[17];

// // //   if (halamanSatu) {
// // //     // Halaman 1: Suhu, Kelembapan, Tanah & Status Pompa
// // //     if (isnan(suhu) || isnan(kelembapan)) {
// // //       tampilBaris(0, "DHT Error");
// // //     } else {
// // //       snprintf(isiLcd, sizeof(isiLcd), "T:%.1f%cC H:%.0f%%", suhu, 223, kelembapan);
// // //       tampilBaris(0, isiLcd);
// // //     }
// // //     snprintf(isiLcd, sizeof(isiLcd), "Soil:%2d%% Pmp:%s", tanah, pompaNyala ? "ON" : "OFF");
// // //     tampilBaris(1, isiLcd);
// // //   } else {
// // //     // Halaman 2: UV, LDR & Status Grow Light
// // //     snprintf(isiLcd, sizeof(isiLcd), "UV:%.2f LDR:%d%%", uvIndex, ldrPersen);
// // //     tampilBaris(0, isiLcd);
// // //     snprintf(isiLcd, sizeof(isiLcd), "GrowLight : %s", lampuNyala ? "ON" : "OFF");
// // //     tampilBaris(1, isiLcd);
// // //   }
// // //   halamanSatu = !halamanSatu;

// // //   // 6. LOG SERIAL MONITOR
// // //   Serial.printf("Soil: %d%% (%s) -> Pompa: %s | LDR: %d%%, UV: %.2f -> Lampu: %s | T: %.1fC, H: %.0f%%\n",
// // //                 tanah, statusTanah(tanah), pompaNyala ? "ON" : "OFF",
// // //                 ldrPersen, uvIndex, lampuNyala ? "ON" : "OFF",
// // //                 isnan(suhu) ? 0.0 : suhu, isnan(kelembapan) ? 0.0 : kelembapan);

// // //   delay(2000);
// // // }

// // #include <Arduino.h>

// // #define RELAY_PUMP_PIN 13

// // void setup()
// // {
// //     Serial.begin(115200);

// //     pinMode(
// //         RELAY_PUMP_PIN,
// //         OUTPUT
// //     );

// //     digitalWrite(
// //         RELAY_PUMP_PIN,
// //         HIGH
// //     );

// //     delay(3000);
// // }

// // void loop()
// // {
// //     Serial.println("GPIO13 = LOW");
// //     digitalWrite(
// //         RELAY_PUMP_PIN,
// //         LOW
// //     );

// //     delay(5000);

// //     Serial.println("GPIO13 = HIGH");
// //     digitalWrite(
// //         RELAY_PUMP_PIN,
// //         HIGH
// //     );

// //     delay(5000);
// // }


// /*
//  * Tes: Relay + Pompa + Soil Moisture + LCD 16x2 I2C (ESP32)
//  * Pin disalin dari kode lama:
//  *   Relay Pompa  : GPIO 13
//  *   Soil AOUT    : GPIO 34 (ADC1)
//  *   Soil VCC     : GPIO 25 (dinyalakan hanya saat baca)
//  *   LCD I2C      : SDA 21, SCL 22, alamat 0x27
//  */

// #include <Arduino.h>
// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>

// // ---------- Pin ----------
// #define RELAY_PUMP_PIN  13
// #define SOIL_PIN        34
// #define SOIL_PWR        25
// #define I2C_SDA         21
// #define I2C_SCL         22
// #define LCD_ADDR        0x27

// // ---------- Polaritas relay ----------
// // false = HIGH menyalakan pompa, true = LOW menyalakan pompa
// // Kalau pompa terbalik (nyala saat harusnya mati), ubah nilai ini.
// #define PUMP_ACTIVE_LOW  true
// #define PUMP_LEVEL_ON    (PUMP_ACTIVE_LOW ? LOW  : HIGH)
// #define PUMP_LEVEL_OFF   (PUMP_ACTIVE_LOW ? HIGH : LOW)

// // ---------- Kalibrasi soil moisture (sesuaikan dengan sensor Anda) ----------
// #define SOIL_DRY   0       // nilai ADC di udara
// #define SOIL_WET   2450    // nilai ADC saat dicelup air

// // ---------- Ambang batas (hysteresis) ----------
// #define BATAS_POMPA_ON   30   // pompa nyala jika tanah < 30%
// #define BATAS_POMPA_OFF  70   // pompa mati jika tanah > 70%

// LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
// bool pompaNyala = false;

// void tampilBaris(uint8_t baris, const char *teks) {
//   char buf[17];
//   snprintf(buf, sizeof(buf), "%-16s", teks);
//   lcd.setCursor(0, baris);
//   lcd.print(buf);
// }

// void bacaSoil(int &raw, int &persen) {
//   digitalWrite(SOIL_PWR, HIGH);
//   delay(300);

//   long total = 0;
//   for (int i = 0; i < 10; i++) {
//     total += analogRead(SOIL_PIN);
//     delay(10);
//   }
//   digitalWrite(SOIL_PWR, LOW);

//   raw = total / 10;
//   persen = constrain(map(raw, SOIL_DRY, SOIL_WET, 0, 100), 0, 100);
// }

// void setPompa(bool nyala) {
//   pompaNyala = nyala;
//   digitalWrite(RELAY_PUMP_PIN, nyala ? PUMP_LEVEL_ON : PUMP_LEVEL_OFF);
// }

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   // Pastikan pompa MATI sejak awal
//   digitalWrite(RELAY_PUMP_PIN, PUMP_LEVEL_OFF);
//   pinMode(RELAY_PUMP_PIN, OUTPUT);
//   setPompa(false);

//   pinMode(SOIL_PWR, OUTPUT);
//   digitalWrite(SOIL_PWR, LOW);

//   Wire.begin(I2C_SDA, I2C_SCL);
//   lcd.init();
//   lcd.backlight();
//   tampilBaris(0, "Tes Pompa+Soil");
//   tampilBaris(1, "Mulai...");
//   delay(1500);
//   lcd.clear();

//   Serial.println("=== TES RELAY + POMPA + SOIL + LCD ===");
// }

// void loop() {
//   int raw, tanah;
//   bacaSoil(raw, tanah);

//   // Hysteresis: ON < 30%, OFF > 70%
//   if (tanah < BATAS_POMPA_ON) {
//     setPompa(true);
//   } else if (tanah > BATAS_POMPA_OFF) {
//     setPompa(false);
//   }

//   // LCD
//   char isi[17];
//   snprintf(isi, sizeof(isi), "Soil:%3d%% ADC:%d", tanah, raw);
//   tampilBaris(0, isi);
//   snprintf(isi, sizeof(isi), "Pompa: %s", pompaNyala ? "NYALA" : "MATI");
//   tampilBaris(1, isi);

//   // Serial
//   Serial.printf("ADC: %d | Soil: %d%% | Pompa: %s\n",
//                 raw, tanah, pompaNyala ? "NYALA" : "MATI");

//   delay(1000);
// }

/*
 * Smart Indoor Planter - Rule-Based Automation & IoT Telemetry (ESP32)
 * 1. Pompa Air (Pin 13): ON jika Soil < 30%, OFF jika Soil > 70% (Hysteresis)
 * 2. Grow Light (Pin 26): ON jika LDR < 60%, OFF jika LDR > 80% (Hysteresis)
 * 3. LCD 16x2 menampilkan status sensor dan aktuator secara bergantian.
 * 4. Kirim data telemetri secara real-time ke Backend API via HTTP POST.
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ---------- Konfigurasi Wi-Fi & Server Backend ----------
const char* ssid       = "Jojo";              // Sesuai nama hotspot HP kamu
const char* password   = "Jojoo1234";         // Password hotspot HP kamu

// URL Backend menggunakan IPv4 laptop kamu (port 4000)
const char* serverUrl  = "http://172.20.10.5:4000/api/telemetry"; 

// ---------- Definisi Pin ----------
#define DHT_PIN         4        // DATA DHT11
#define DHT_TYPE        DHT11
#define SOIL_PIN        34       // AOUT Soil Moisture (ADC1)
#define UV_PIN          35       // AOUT Sensor UV GUVA-S12SD (ADC1)
#define LDR_PIN         32       // AOUT Modul LDR (ADC1)
#define SOIL_PWR        25       // VCC Soil Moisture (aktif saat baca)
#define RELAY_LIGHT_PIN 26       // IN Relay 1 (Grow Light)
#define RELAY_PUMP_PIN  13       // IN Relay 2 (Pompa Air / Dinamo)

#define I2C_SDA         21
#define I2C_SCL         22
#define LCD_ADDR        0x27

// ---------- Konfigurasi Relay Grow Light (Pin 26) ----------
#define LIGHT_ACTIVE_LOW  false
#define LIGHT_LEVEL_ON   (LIGHT_ACTIVE_LOW ? LOW  : HIGH)
#define LIGHT_LEVEL_OFF  (LIGHT_ACTIVE_LOW ? HIGH : LOW)

// ---------- Konfigurasi Relay Pompa Air (Pin 13) ----------
// LOGIKA NO (Normally Open): Diubah menjadi true sesuai dengan kode tes yang berhasil
#define PUMP_ACTIVE_LOW  true
#define PUMP_LEVEL_ON    (PUMP_ACTIVE_LOW ? LOW  : HIGH)
#define PUMP_LEVEL_OFF   (PUMP_ACTIVE_LOW ? HIGH : LOW)

// ---------- Kalibrasi Soil Moisture ----------
#define SOIL_DRY        0        // Nilai analog di udara terbuka
#define SOIL_WET        2450     // Nilai analog saat tercelup air

// ---------- Ambang Batas (Threshold) Otomasi & Hysteresis ----------
#define BATAS_POMPA_ON   30      // Pompa nyala jika tanah < 30%
#define BATAS_POMPA_OFF  70      // Pompa mati jika tanah > 70%

#define BATAS_LDR_ON     60      // Lampu nyala jika cahaya LDR < 60% (Gelap)
#define BATAS_LDR_OFF    80      // Lampu mati jika cahaya LDR > 80% (Terang)

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// Variabel penyimpan status aktuator
bool pompaNyala = false;
bool lampuNyala = false;

// ---------------------------------------------------------------
// Fungsi Pembantu Tampilan & Sensor
// ---------------------------------------------------------------
void tampilBaris(uint8_t baris, const char *teks)
{
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", teks);
  lcd.setCursor(0, baris);
  lcd.print(buf);
}

int bacaTanahPersen()
{
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

const char *statusTanah(int persen)
{
  if (persen < BATAS_POMPA_ON)   return "KERING";
  if (persen <= BATAS_POMPA_OFF) return "LEMBAB";
  return "BASAH";
}

// ---------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  delay(1000);

  // Pastikan kedua relay MATI saat pertama kali menyala
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

  lcd.init();
  lcd.backlight();
  tampilBaris(0, "Smart Planter");
  tampilBaris(1, "Connecting WiFi");

  // Koneksi Wi-Fi
  WiFi.begin(ssid, password);
  Serial.printf("Menghubungkan ke Wi-Fi: %s\n", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Terhubung!");
  Serial.print("IP Address ESP32: ");
  Serial.println(WiFi.localIP());

  tampilBaris(1, "WiFi Connected");
  delay(2000);
  lcd.clear();

  Serial.println("=== SISTEM OTOMASI SMART INDOOR PLANTER AKTIF ===");
}

void loop()
{
  // 1. BACA SEMUA SENSOR
  float suhu       = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  int   tanah      = bacaTanahPersen();
  
  // Baca LDR (0% = Gelap, 100% = Terang)
  int   ldrRaw     = analogRead(LDR_PIN);
  int   ldrPersen  = map(ldrRaw, 4095, 0, 0, 100);
  ldrPersen        = constrain(ldrPersen, 0, 100);
  
  // Baca UV Index (Pemantauan)
  int   uvRaw      = analogRead(UV_PIN);
  float uvVoltage  = (uvRaw * 3.3) / 4095.0;
  float uvIndex    = uvVoltage / 0.1; 

  // 2. LOGIKA OTOMASI POMPA AIR (Hysteresis: ON < 30%, OFF > 70%)
  if (tanah < BATAS_POMPA_ON) {
    pompaNyala = true;  // Tanah kering (< 30%), nyalakan pompa
  } else if (tanah > BATAS_POMPA_OFF) {
    pompaNyala = false; // Tanah sudah basah (> 70%), matikan pompa
  }

  digitalWrite(RELAY_PUMP_PIN, pompaNyala ? PUMP_LEVEL_ON : PUMP_LEVEL_OFF);

  // 3. LOGIKA OTOMASI GROW LIGHT (Hysteresis: ON < 60%, OFF > 80%)
  if (ldrPersen < BATAS_LDR_ON) {
    lampuNyala = true;  // Ruangan gelap (< 60%), lampu menyala
  } else if (ldrPersen > BATAS_LDR_OFF) {
    lampuNyala = false; // Ruangan terang (> 80%), lampu mati
  }

  digitalWrite(RELAY_LIGHT_PIN, lampuNyala ? LIGHT_LEVEL_ON : LIGHT_LEVEL_OFF);

  // 4. KIRIM DATA KE BACKEND SERVER VIA HTTP POST (REAL-TIME)
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Format JSON payload yang bersih dan valid untuk backend API
    String jsonPayload = "{";
    jsonPayload += "\"deviceId\":\"esp32-planter-01\",";
    jsonPayload += "\"temperature\":" + String(isnan(suhu) ? 0.0 : suhu) + ",";
    jsonPayload += "\"humidity\":" + String(isnan(kelembapan) ? 0.0 : kelembapan) + ",";
    jsonPayload += "\"soilMoisture\":" + String(tanah) + ",";
    jsonPayload += "\"soilStatus\":\"" + String(statusTanah(tanah)) + "\",";
    jsonPayload += "\"ldrLight\":" + String(ldrPersen) + ",";
    jsonPayload += "\"uvIndex\":" + String(uvIndex) + ",";
    jsonPayload += "\"pumpOn\":" + String(pompaNyala ? "true" : "false") + ",";
    jsonPayload += "\"growlightOn\":" + String(lampuNyala ? "true" : "false");
    jsonPayload += "}";

    int httpResponseCode = http.POST(jsonPayload);
    if (httpResponseCode > 0) {
      Serial.printf("Telemetri terkirim! HTTP Response code: %d\n", httpResponseCode);
    } else {
      Serial.printf("Gagal mengirim data, error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  } else {
    Serial.println("Wi-Fi terputus, mencoba menghubungkan ulang...");
    WiFi.reconnect();
  }

  // 5. TAMPILAN LCD (Bergantian tiap 2 detik)
  static bool halamanSatu = true;
  char isiLcd[17];

  if (halamanSatu) {
    // Halaman 1: Suhu, Kelembapan, Tanah & Status Pompa
    if (isnan(suhu) || isnan(kelembapan)) {
      tampilBaris(0, "DHT Error");
    } else {
      snprintf(isiLcd, sizeof(isiLcd), "T:%.1f%cC H:%.0f%%", suhu, 223, kelembapan);
      tampilBaris(0, isiLcd);
    }
    snprintf(isiLcd, sizeof(isiLcd), "Soil:%2d%% Pmp:%s", tanah, pompaNyala ? "ON" : "OFF");
    tampilBaris(1, isiLcd);
  } else {
    // Halaman 2: UV, LDR & Status Grow Light
    snprintf(isiLcd, sizeof(isiLcd), "UV:%.2f LDR:%d%%", uvIndex, ldrPersen);
    tampilBaris(0, isiLcd);
    snprintf(isiLcd, sizeof(isiLcd), "GrowLight : %s", lampuNyala ? "ON" : "OFF");
    tampilBaris(1, isiLcd);
  }
  halamanSatu = !halamanSatu;

  // 6. LOG SERIAL MONITOR
  Serial.printf("Soil: %d%% (%s) -> Pompa: %s | LDR: %d%%, UV: %.2f -> Lampu: %s | T: %.1fC, H: %.0f%%\n",
                tanah, statusTanah(tanah), pompaNyala ? "ON" : "OFF",
                ldrPersen, uvIndex, lampuNyala ? "ON" : "OFF",
                isnan(suhu) ? 0.0 : suhu, isnan(kelembapan) ? 0.0 : kelembapan);

  delay(2000);
}