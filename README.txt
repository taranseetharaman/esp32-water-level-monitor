# ESP32 Water Level Monitor (Blynk + OTA)

Monitor your water tank using three float sensors and an ESP32 board.  
Shows the level in the Blynk app, controls a relay for the pump, and supports OTA (Over-The-Air) firmware updates.

---

## ⚙️ Features
- Levels: 100 %, 75 %, 50 %, 25 %, and **critical 15 %**
- LEDs: Green (100/75 blink), Blue (50 solid), Red (25 solid), Red blink (15 %)
- Blynk virtual pins:
  - **V1** → Water level %
  - **V2** → Motor ON/OFF status
- Auto motor OFF at 15 %
- OTA update window: 5 minutes after boot

---

## 🧠 Hardware
| Function | ESP32 Pin |
|-----------|-----------|
| Float LOW | 32 |
| Float MID | 33 |
| Float HIGH| 25 |
| Relay (active LOW)| 26 |
| LED RED   | 14 |
| LED BLUE  | 27 |
| LED GREEN | 12 |

---

## 🌐 Blynk Setup
1. Create a Template (note Template ID & Name).  
2. Add Datastreams → V1 (Integer = level %), V2 (String = motor status).  
3. Add Events → `low_water`, `critical_water`.  
4. Create Device → copy Auth Token (for config.h).

---

## 🧩 File Layout
