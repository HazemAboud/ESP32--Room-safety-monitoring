# Introduction

The developed system monitors environmental safety by collecting sound levels, gas and smoke concentration, and temperature, the raw data is then transformed into meaningful textual output, and combined to calculate room safety status based on a rule set. The output is displayed in an OLED screen that is activated by sound in addition to a web page accessible through AP mode.

# Devices
| No. | Component | ESP32 Pin | Description |
|:---:|:---|:---:|:---|
| 1 | **ESP32 (30-pin)** | -- | Central microcontroller unit. |
| 2 | **MQ2 Gas Sensor** | **GPIO 34** | Analog input for detecting gas and smoke levels. |
| 3 | **DHT11 Sensor** | **GPIO 4** | Digital input for temperature and humidity data. |
| 4 | **KY037 Microphone** | **GPIO 35** | Analog input used to detect sound and wake the display. |
| 5 | **SSD1306 OLED** | **SDA: 21 / SCL: 22** | I2C display for real-time system status and readings. |

---
# Design
<img width="948" height="531" alt="image" src="https://github.com/user-attachments/assets/c47671c6-813e-4afe-b00f-1accea03efaa" />

<img width="484" height="645" alt="image" src="https://github.com/user-attachments/assets/75f70d54-315a-492b-9aaf-3b749f3eb379" />

<img width="435" height="580" alt="image" src="https://github.com/user-attachments/assets/8ccc087a-1817-435f-90e2-8d06ff469102" />

<img width="301" height="360" alt="image" src="https://github.com/user-attachments/assets/fdfe2a4a-cc76-4563-b0f8-fe6532ed69b6" />

# Status logic

```
void updateTempStatus(float currentTemp) {
  if (currentTemp >= 20.0 && currentTemp <= 26.0) temp_v = "Optimal";
  else if (currentTemp > 26.0 && currentTemp <= 30.0) temp_v = "Normal";
  else if ((currentTemp >= 10.0 && currentTemp < 20.0) || (currentTemp > 30.0 && currentTemp <= 38.0)) temp_v = "Caution";
  else temp_v = "Danger";
}

String getAirQuality(float raw) {
  if (raw <= 600) return "Normal";
  if (raw <= 1200) return "Caution";
  return "Danger";
}

String getSoundStatus(int ptp) {
  if (ptp < 30) return "Quiet";
  if (ptp < 70) return "Normal";
  if (ptp < 150) return "Caution";
  return "Danger";
}
```
This if for getting the status for each variable which are then combined for the conclusion

```
void calculateRoomStatus() {
  if (temp_v == "Danger" || airText == "Danger" || soundText == "Danger") {
    roomStatus = "Danger";
    return;
  }

  int c_counter = 0;
  if (temp_v == "Caution") c_counter++;
  if (airText == "Caution") c_counter++;
  if (soundText == "Caution") c_counter++;

  if (c_counter >= 2) {
    roomStatus = "Danger";   
  } else if (c_counter == 1) {
    roomStatus = "Caution";
  } else {
    roomStatus = "Normal";
  }
}

```
