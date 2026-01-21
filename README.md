# 🧢 SmurfZone – Smart Parking System

**Real-Time Smart Parking System using ESP32, FreeRTOS, and IoT Dashboard**

A complete **Embedded & IoT project** developed as part of the **Real-Time Operating Systems (RTOS)** course.  
This system manages parking slots in real-time, calculates parking duration and cost, and provides live monitoring via a **web dashboard** and **Telegram Bot notifications**.

---

## 📌 Project Overview
SmurfZone is a **smart parking system** designed to automate entry/exit management, track parking slots, and provide real-time billing for vehicles.  
The system is implemented using **ESP32** and **FreeRTOS**, leveraging **multi-tasking, semaphores, queues, and mutexes** for efficient real-time operation.

**Key Features:**
- Real-time detection of car **entry** and **exit** 🚗  
- Automatic **gate control** (open/close) ⚙️  
- Dynamic **parking slot management** using **Counting Semaphores** 🅿️  
- Parking **duration calculation** ⏱️  
- Automatic **parking fee calculation** 💰  
- Real-time **Telegram Bot notifications** 🤖  
- **Live web dashboard** showing system status, slots, and last exited car 📊  

---

## 🛠 Hardware Components
- **ESP32 microcontroller** 🟢  
- **2 × IR Sensors** (entry & exit detection) 🔵  
- **Servo Motor** for gate control ⚙️  
- **LCD Display** for real-time slot & gate status 💻  
- **2 × LEDs** for availability indication 💡  

---

## 🧠 RTOS Implementation
The system leverages **FreeRTOS** to manage multiple tasks simultaneously:  

| Task           | Purpose |
|----------------|---------|
| SensorTask     | Monitors IR sensors for car entry/exit detection |
| GateTask       | Controls servo motor and manages parking slot occupancy |
| TelegramTask   | Sends parking info (time & price) to Telegram Bot |
| LCDTask        | Updates the LCD display with available slots and gate status |
| WebTask        | Runs embedded web server for live dashboard |

**Synchronization & Communication:**
- **Counting Semaphore:** Tracks available parking slots  
- **Queues:** Send events between tasks (entry/exit & Telegram messages)  
- **Mutex:** Protects LCD access from concurrent tasks  

---

## 🌐 IoT Integration
- **Live Web Dashboard** displaying:  
  - Available slots  
  - Gate status  
  - Slot occupancy  
  - Last exited car info (ID, time, price)  
- **Telegram Bot Notifications:** Sends automatic messages with:  
  - Car ID  
  - Parking duration  
  - Parking cost  

---

## 👥 Project Team
- **Mohamed Zaky** – Project Lead  
- **Mohamed Ragab**  
- **Youssef El-Demerdash**  
- **Mostafa Ashraf**  
- **Mohamed Waleed**  

---

## 💻 How to Run
1. Install **Arduino IDE**  
2. Add **ESP32 board** via Board Manager  
3. Open `SmurfZone.ino` file  
4. Connect hardware as described:  
   - IR_ENTRY → GPIO 18  
   - IR_EXIT → GPIO 34  
   - Servo → GPIO 25  
   - LEDs → GPIO 26 (Green), GPIO 27 (Red)  
   - LCD → I2C (0x27)  
5. Upload the code to **ESP32**  
6. Connect to WiFi (`username / password`)  
7. Open the Web Dashboard using ESP32 IP in browser  
8. Monitor **Telegram Bot** for real-time car exit messages  

---

## 📂 Repository Structure
```
SmurfZone/
│
├─ SmurfZone.ino          # Main Arduino code
├─ README.md              # Project documentation
└─ screenshots/
   ├─ dashboard.png
   └─ telegram.png

```

---

## 💡 Future Improvements
- Mobile app integration for real-time slot booking  
- Dynamic pricing based on peak/off-peak hours  
- Camera-based license plate recognition  
- Cloud-based analytics for multiple parking sites  

---

**SmurfZone – Smart Parking System** combines **Embedded Systems, FreeRTOS, IoT, and real-time automation** into one cohesive project. 🚀  




