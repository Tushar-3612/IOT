PRoject/ Like that way and write the name in vscode 
│── SmartVentilation.ino

│── index.html

│── script.js

│── style.css

│── README.md        # project overview

│── docs/│   
   
   └── README.md   # theory notes


# 📖 IoT Smart Ventilation – Theory Notes

## 🔹 Introduction
This project is designed to **improve indoor air quality** using IoT + sensors.  
It monitors **temperature, humidity, and air quality** in real-time, and controls the **fan/ventilation system** automatically.

---

## 🔹 Sensors Used
- 🌡️ **DHT11/DHT22** → Measures temperature & humidity.  
- 🏭 **MQ135** → Detects air quality (smoke, CO₂, harmful gases).  

---

## 🔹 Working Principle
1. Sensors detect environment conditions.  
2. Data is sent to the **Arduino/ESP32** microcontroller.  
3. If values cross a threshold:  
   - Ventilation/Fan turns **ON**.  
   - System can also push data to IoT cloud or dashboard.  
4. User can **visualize live readings** on the web interface.  

---

## 🔹 Applications
- 🏠 Smart homes & offices  
- 🏥 Hospitals & laboratories  
- 🏭 Industrial workspaces  
- 🎓 Schools & libraries  

---

## 🔹 Future Improvements
- 📱 Mobile app integration for remote monitoring  
- 🤖 AI-based prediction of air quality trends  
- ☀️ Solar-powered design for sustainability  

---

## 📸 Project Snapshots

### 🔹 Before Connection (ESP32 WiFi not yet implemented)
<img width="800" alt="Before ESP32 connection" src="https://github.com/user-attachments/assets/d4a02590-e475-4332-b39a-d280e131aac1" />

---

### 🔹 After Connection (ESP32 + WiFi working)
![After Connection](https://github.com/user-attachments/assets/8ef734c6-eca1-42a1-8ab0-84d7c8d5005f)

---

✅ These notes explain the theory part for documentation and reports.  
👉 For code, check the main [`SmartVentilation.ino`](../SmartVentilation.ino) file and the web dashboard files (`index.html`, `script.js`, `style.css`).  

