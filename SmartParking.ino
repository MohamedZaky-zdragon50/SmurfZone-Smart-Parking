// ==================================================
// Smart Parking System - Powered By SmurfZone 
// ==================================================

// ===================== Libraries =====================
// I2C communication and LCD control
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Servo motor control
#include <ESP32Servo.h>

// WiFi and secure client libraries
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Telegram Bot library
#include <UniversalTelegramBot.h>

// FreeRTOS core libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// ESP32 Web Server
#include <WebServer.h>

// ================= WiFi credentials =================

#define WIFI_SSID     "Good Good"
#define WIFI_PASSWORD "Good9876_4321goodzaky"

// ================= Telegram =================
#define BOT_TOKEN "8582571727:AAEv6B6SXq1TZUuKzhtib78ykhc88Y4O_Rw"
#define CHAT_ID   "1153208115"

// Telegram root certificate for secure connection
extern const char TELEGRAM_CERTIFICATE_ROOT[] asm("_binary_telegram_cert_pem_start");

// Secure client and bot object
WiFiClientSecure telegramClient;
UniversalTelegramBot bot(BOT_TOKEN, telegramClient);

// ================= Hardware =================
#define IR_ENTRY   18
#define IR_EXIT    34
#define SERVO_PIN  25
#define GREEN_LED  26
#define RED_LED    27

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo barrierServo; // Servo motor object

// ================= RTOS =================
QueueHandle_t eventQueue; 
QueueHandle_t telegramQueue;
SemaphoreHandle_t slotsSemaphore; // Semaphore representing available parking slots
SemaphoreHandle_t lcdMutex; // Mutex to protect LCD access between tasks

// ================= System =================
const int totalSlots = 4;
String gateStatus = "Closed";

// Flags to prevent overlapping entry/exit processes
bool enteringProcess = false;
bool exitingProcess  = false;

// ================= Parking Data =================
struct CarInfo {
  bool occupied;
  unsigned long entryTime;
};

CarInfo spots[4];

// Information about the last exited car
int lastCarID = -1;
unsigned long lastCarTime = 0;
int lastCarPrice = 0;

// ================= Telegram Struct =================
struct TelegramMsg {
  int carID;
  unsigned long timeSec;
  int price;
};

// ================= Events =================
enum EventType { ENTRY, EXIT };
struct CarEvent { EventType type; };

// ================= LEDs =================
// Update LEDs based on available parking slots
void updateLEDs() {
  int freeSlots = uxSemaphoreGetCount(slotsSemaphore);
  digitalWrite(GREEN_LED, freeSlots > 0);
  digitalWrite(RED_LED,   freeSlots == 0);
}

// ================= Price =================
int calculatePrice(unsigned long seconds) {
  int blocks = seconds / 5;
  if (seconds % 5) blocks++;
  if (blocks < 1) blocks = 1;
  return blocks * 10;
}

// ================= WiFi + Telegram =================
void connectToWiFiAndTelegram() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  telegramClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}

// ================= Web Task =================
// port 80
WebServer server(80);

// handling HTTP client requests
void WebTask(void *pv) {
  while (1) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ================= Web API =================

void handleData() {
  String json = "{";

  json += "\"available\":" + String(uxSemaphoreGetCount(slotsSemaphore)) + ",";
  json += "\"gate\":\"" + gateStatus + "\",";
  json += "\"lastCarID\":" + String(lastCarID) + ",";
  json += "\"lastTime\":" + String(lastCarTime) + ",";
  json += "\"lastPrice\":" + String(lastCarPrice) + ",";
  json += "\"spots\":[";

  for (int i = 0; i < totalSlots; i++) {
    json += spots[i].occupied ? "1" : "0";
    if (i < totalSlots - 1) json += ",";
  }

  json += "]}";

  server.send(200, "application/json", json);
}

// ================= HTML Dashboard =================
// ================= SmurfZone Dashboard =================
String getHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>SmurfZone Parking</title>
<link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css" rel="stylesheet">
<style>
/* ===== Global ===== */
body{
  margin:0;
  font-family: "Segoe UI", Tahoma, Arial, sans-serif;
  background: linear-gradient(135deg,#0f172a,#1e293b);
  color:#e5e7eb;
}
/* ===== Header ===== */
.header{
  background:#020617;
  padding:22px 30px;
  display:flex;
  align-items:center;
  justify-content:center;
  gap:15px;
  box-shadow:0 6px 18px rgba(0,0,0,0.6);
}
.logo{
  font-size:34px;
}
.header h1{
  margin:0;
  font-size:26px;
  letter-spacing:1px;
  color:#38bdf8;
}
/* ===== Container ===== */
.container{
  padding:35px 20px;
  max-width:1100px;
  margin:auto;
  min-height:calc(100vh - 160px);
}
/* ===== Parking Spots ===== */
.spots{
  display:grid;
  grid-template-columns:repeat(auto-fit,minmax(140px,1fr));
  gap:28px;
  margin-top:30px;
}
.spot{
  height:150px;
  border-radius:22px;
  display:flex;
  flex-direction:column;
  align-items:center;
  justify-content:center;
  gap:10px;
  font-size:18px;
  font-weight:600;
  letter-spacing:0.5px;
  transition:all 0.3s ease;
  box-shadow:0 8px 20px rgba(0,0,0,0.35);
  position:relative;
}
/* Car Emoji */
.spot i{
  font-size:42px;
  margin-bottom:6px;
  opacity:0.4;
  transition:all 0.3s ease;
}
/* Free */
.free i{
  opacity:0.35;
}
/* Busy */
.busy i{
  opacity:1;
  transform:scale(1.25);
  color:#064e3b;
}
/* Free Slot */
.free{
  background:linear-gradient(145deg,#334155,#475569);
  color:#e5e7eb;
}
.free::before{
  opacity:0.35;
}
/* Busy Slot */
.busy{
  background:linear-gradient(145deg,#4ade80,#22c55e);
  color:#f0fdf4;
  box-shadow:0 0 14px rgba(74,222,128,0.45);
  transform:scale(1.04);
}
.busy::before{
  opacity:1;
  transform:scale(1.2);
}
/* Hover */
.spot:hover{
  transform:translateY(-6px);
}
/* ===== Cards ===== */
.cards{
  display:grid;
  grid-template-columns:repeat(auto-fit,minmax(260px,1fr));
  gap:28px;
  margin-top:55px;
}
.card{
  background:linear-gradient(145deg,#020617,#0f172a);
  padding:26px;
  border-radius:20px;
  box-shadow:0 10px 25px rgba(0,0,0,0.55);
  transition:0.3s;
}
.card:hover{
  transform:translateY(-6px);
}
.card h3{
  margin-top:0;
  margin-bottom:16px;
  color:#38bdf8;
  font-size:18px;
  letter-spacing:0.5px;
}
.card p{
  margin:8px 0;
  font-size:15px;
}
/* Highlighted Values */
#gate,
#cid,
#ctime,
#cprice{
  font-weight:600;
  color:#fbbf24;
}
/* ===== Footer ===== */
.footer{
  background:#020617;
  text-align:center;
  padding:16px;
  font-size:14px;
  color:#94a3b8;
  box-shadow:0 -4px 14px rgba(0,0,0,0.45);
}
.footer span{
  color:#38bdf8;
  font-weight:600;
}
</style>
</head>
<body>
<!-- ===== Header ===== -->
<div class="header">
  <div class="logo">🧢</div>
  <h1>SmurfZone Parking</h1>
</div>
<!-- ===== Content ===== -->
<div class="container">
  <div class="spots">
<div id="s1" class="spot free">
  <i class="fas fa-car"></i>
  Spot 1
</div>
<div id="s2" class="spot free">
  <i class="fas fa-car"></i>
  Spot 2
</div>
<div id="s3" class="spot free">
  <i class="fas fa-car"></i>
  Spot 3
</div>
<div id="s4" class="spot free">
  <i class="fas fa-car"></i>
  Spot 4
</div>
  </div>
  <div class="cards">
    <div class="card">
      <h3>Gate Status</h3>
      <p id="gate">Closed</p>
    </div>
    <div class="card">
      <h3>Last Car</h3>
      <p>ID: <span id="cid">-</span></p>
      <p>Time: <span id="ctime">0</span> sec</p>
      <p>Price: <span id="cprice">0</span> EGP</p>
    </div>
  </div>
</div>
<!-- ===== Footer ===== -->
<div class="footer">
  © ZAKY <span>SmurfZone</span> Smart Parking System
</div>
<script>
async function update(){
  let r = await fetch("/data");
  let d = await r.json();
  for(let i=1;i<=4;i++){
    let s=document.getElementById("s"+i);
    s.className = d.spots[i-1] ? "spot busy" : "spot free";
  }
  document.getElementById("gate").innerText = d.gate;
  document.getElementById("cid").innerText = d.lastCarID;
  document.getElementById("ctime").innerText = d.lastTime;
  document.getElementById("cprice").innerText = d.lastPrice;
}
setInterval(update,1000);
update();
</script>
</body>
</html>
)rawliteral";
}

// ================= Tasks =================

// ================= Sensor Task =================
void SensorTask(void *pv) {
  bool entryFlag = false, exitFlag = false; // Flags to avoid repeated triggering

  while (1) {
    bool entry = digitalRead(IR_ENTRY) == LOW;
    bool exitS = digitalRead(IR_EXIT)  == LOW;

    // Get current number of available slots (slotsSemaphore)
    int available = uxSemaphoreGetCount(slotsSemaphore);

    // Conditions:
    // - Entry sensor active
    // - Exit sensor inactive
    // - Not already processed
    // - Free slots available
    // - No exit operation in progress
    if (entry && !exitS && !entryFlag && available > 0 && !exitingProcess) {
      entryFlag = true;
      CarEvent ev = { ENTRY };
      xQueueSend(eventQueue, &ev, portMAX_DELAY);
    }


    // -------- EXIT detection --------
    // Conditions:
    // - Exit sensor active
    // - Entry sensor inactive
    // - Parking not empty
    // - No entry operation in progress
    if (exitS && !entry && !exitFlag && available < totalSlots && !enteringProcess) {
      exitFlag = true;
      CarEvent ev = { EXIT };
      xQueueSend(eventQueue, &ev, portMAX_DELAY);
    }

    // Reset flags when sensors are released    
    if (!entry) entryFlag = false;
    if (!exitS) exitFlag = false;

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================= Gate Task =================
void GateTask(void *pv) {
  CarEvent ev;

  while (1) {

    // Wait for an event from SensorTask
    if (xQueueReceive(eventQueue, &ev, portMAX_DELAY)) {

      // ENTRY
      if (ev.type == ENTRY) {
        enteringProcess = true;

        // Find the first free slot
        if (xSemaphoreTake(slotsSemaphore, 0)) {
          for (int i = 0; i < totalSlots; i++) {
            if (!spots[i].occupied) {
              spots[i].occupied = true;
              spots[i].entryTime = millis();
              break;
            }
          }
          // Update system state
          updateLEDs();
          gateStatus = "Open";

          barrierServo.write(0);
          vTaskDelay(pdMS_TO_TICKS(2000));
          barrierServo.write(90);
          gateStatus = "Closed";
        }
        enteringProcess = false;
      }

      // EXIT
      if (ev.type == EXIT) {
        exitingProcess = true;

        int oldest = -1;
        unsigned long oldestTime = ULONG_MAX;

        // Find the car that entered earliest
        for (int i = 0; i < totalSlots; i++) {
          if (spots[i].occupied && spots[i].entryTime < oldestTime) {
            oldestTime = spots[i].entryTime;
            oldest = i;
          }
        }

        if (oldest != -1) {
          // Calculate parking duration
          unsigned long t = (millis() - spots[oldest].entryTime) / 1000;
          int price = calculatePrice(t);

          // Update dashboard data
          lastCarID = oldest + 1;
          lastCarTime = t;
          lastCarPrice = price;
          
          // Send data to Telegram task
          TelegramMsg msg = { lastCarID, t, price };
          xQueueSend(telegramQueue, &msg, portMAX_DELAY);

          // Free the parking slot
          spots[oldest].occupied = false;
          xSemaphoreGive(slotsSemaphore);
        }

        updateLEDs();
        gateStatus = "Open";
        barrierServo.write(0);
        vTaskDelay(pdMS_TO_TICKS(2000));
        barrierServo.write(90);
        gateStatus = "Closed";

        exitingProcess = false;
      }
    }
  }
}

// ================= Telegram Task =================
void TelegramTask(void *pv) {
  TelegramMsg msg;

  while (1) {
    if (xQueueReceive(telegramQueue, &msg, portMAX_DELAY)) {

      String text =
        "🚗 Car Exited Parking\n"
        "🆔 Car ID: " + String(msg.carID) + "\n"
        "⏱ Time: " + String(msg.timeSec) + " sec\n"
        "💰 Price: " + String(msg.price) + " EGP";

      bot.sendMessage(CHAT_ID, text, "");

      // Delay to avoid Telegram flooding
      vTaskDelay(pdMS_TO_TICKS(1500));  // مهم 
    }
  }
}

// -------- Logger Task --------
// void LoggerTask(void *pv) {
//   while (1) {
//     Serial.print("Slots: ");
//     Serial.print(uxSemaphoreGetCount(slotsSemaphore));
//     Serial.print(" | Gate: ");
//     Serial.println(gateStatus);
//     vTaskDelay(pdMS_TO_TICKS(2000));
//   }
// }
// ================= LCD Task =================
void LCDTask(void *pv) {
  while (1) {
    
    xSemaphoreTake(lcdMutex, portMAX_DELAY); // Lock LCD
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Slots:");
    lcd.print(uxSemaphoreGetCount(slotsSemaphore));
    lcd.print("/");
    lcd.print(totalSlots);
    lcd.setCursor(0,1);
    lcd.print("Gate:");
    lcd.print(gateStatus);
    xSemaphoreGive(lcdMutex); // Unlock LCD
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}


// ================= Boot LCD Message =================
void showBootMessage() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Smart Parking");
  lcd.setCursor(0,1);
  lcd.print("System Starting");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Powered By");
  lcd.setCursor(0,1);
  lcd.print("SmurfZone");
  delay(2000);

  lcd.clear();
}

// ================= Setup =================
// System initialization 
void setup() {
  Serial.begin(115200);

  // Configure hardware pins
  pinMode(IR_ENTRY, INPUT_PULLUP);
  pinMode(IR_EXIT, INPUT_PULLUP);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  // Show boot message
  showBootMessage();
  
   // Initialize servo
  barrierServo.attach(SERVO_PIN);
  barrierServo.write(90);

  connectToWiFiAndTelegram();

  // Debug output
  Serial.println();
  Serial.println("✅ WiFi Connected");
  Serial.print("🌐 Dashboard URL: http://");
  Serial.println(WiFi.localIP());

  // Create RTOS objects
  eventQueue     = xQueueCreate(10, sizeof(CarEvent));
  telegramQueue  = xQueueCreate(5, sizeof(TelegramMsg));
  slotsSemaphore = xSemaphoreCreateCounting(totalSlots, totalSlots);
  lcdMutex       = xSemaphoreCreateMutex();

  // Initialize parking slots
  for (int i = 0; i < totalSlots; i++) {
  spots[i].occupied = false;
  spots[i].entryTime = 0;
}

  //Configure web server routes
  server.on("/", []() {
    server.send(200, "text/html", getHTML());
  });

  server.on("/data", handleData);
  server.begin();

  //Create FreeRTOS tasks
  xTaskCreate(WebTask,      "Web",      4096, NULL, 1, NULL);
  xTaskCreate(SensorTask,   "Sensor",   2048, NULL, 2, NULL);
  xTaskCreate(GateTask,     "Gate",     4096, NULL, 2, NULL);
  xTaskCreate(TelegramTask, "Telegram", 8192, NULL, 1, NULL);
  xTaskCreate(LCDTask,      "LCD",      2048, NULL, 1, NULL);
}

// Main loop is unused
void loop() {
  vTaskDelay(portMAX_DELAY);
}
