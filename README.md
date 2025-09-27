# 🌬️ IoT Smart Ventilation System

## 📌 Project Overview
This project is an **IoT-based Smart Ventilation System** designed to monitor and improve indoor air quality.  
It uses sensors to detect temperature, humidity, and air quality, then automatically adjusts the ventilation system to maintain a comfortable and healthy environment.

---

## 🛠️ Features
- 🌡️ **Temperature Monitoring** → Checks how hot or cold the room is.  
- 💧 **Humidity Monitoring** → Measures moisture level in the air.  
- 🏭 **Air Quality Detection** → Detects dust, smoke, or harmful gases.  
- 🔗 **IoT Connectivity** → Sends real-time data to the cloud/dashboard.  
- ⚡ **Automation** → Triggers fans/vents when air quality or humidity crosses a threshold.  
- 📊 **Data Visualization** → Users can track readings in real-time.

---

## 🧰 Components Used
- **Microcontroller**: ESP32 / Arduino (depending on implementation)  
- **Sensors**:
  - DHT11/DHT22 → Temperature & Humidity  
  - MQ135 → Air Quality (smoke, CO2, gases)  
- **Actuators**:
  - Cooling fan / Vent motor  
- **Cloud/Server**: Thingspeak / Firebase / MQTT broker (optional)  

---

## ⚙️ Working Principle
1. Sensors continuously collect data from the environment.  
2. Data is processed by the microcontroller.  
3. If values exceed safe limits:
   - Fan/ventilation system turns ON.  
   - Alerts/notifications can be sent to the user (via app/cloud).  
4. Data is stored and visualized for monitoring.  

---


