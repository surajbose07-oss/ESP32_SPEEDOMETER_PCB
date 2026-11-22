# ESP32 Portable GPS Speedometer PCB

A custom *70.0 mm × 47.5 mm, 2-layer PCB designed in **KiCad 10* for a portable GPS speedometer powered by a *30-pin ESP32 DevKit*, complete with live OLED telemetry, rotary encoder menu navigation, web UI, and Telegram alerts.

---

## Key Features
* *Real-Time GPS Speed & Distance:* Reads NMEA satellite data via UART (TinyGPS++) to calculate live speed (km/h) and trip metrics.
* *On-Board OLED Telemetry:* Displays crisp speed and status readouts on a 0.96" I2C OLED screen (SSD1306).
* *Physical Controls:* Rotary encoder (KY-040) and tactile push button (SW1) for menu switching and trip resets.
* *Web UI & Notifications:* Built-in ESP32 web dashboard and Telegram alert integration over Wi-Fi.
* *Fabrication-Ready PCB:* 2-layer FR-4 layout (70.0 mm × 47.5 mm) verified with *0 DRC errors and 0 warnings*.

---

## Hardware & Bill of Materials (BOM)
* *Estimated Prototype Cost:* ~₹1,094 (excluding custom PCB fabrication)
* *Full Interactive BOM:* [View Bill of Materials (GPS MODULE_BILL_OF_MATERIAL.csv)](GPS%20MODULE_BILL_OF_MATERIAL.csv)

| Ref | Component | Specification / Footprint | Qty | Est. Cost |
| :--- | :--- | :--- | :---: | :---: |
| ESP_1 | 30-Pin ESP32 DevKit V1 | Custom 30-Pin Footprint (2.54 mm pitch) | 1 | ₹509 |
| GPS_MODULE1 | UART GPS Receiver Module | 1×4 Pin Header (2.54 mm) | 1 | ₹300 |
| 0.96_OLED_SCREEN1 | 0.96" I2C OLED Display | 1×4 Pin Header (2.54 mm) | 1 | ₹200 |
| ROTARY_MODULE1 | Rotary Encoder Module | 1×5 Pin Header (2.54 mm) | 1 | ₹80 |
| SW1 | Tactile Push Button | SW_PUSH_6mm (Through-Hole) | 1 | ₹5 |

---

## PCB Layout Preview

### Front View (F.Cu & Silkscreen)
![PCB Front View](GPS%20MODULE_pcb.png)

### Back View (B.Cu Routing)
![PCB Back View](GPS%20MODULE_pcb_BACK.png)

---

## Repository Contents
* *[Firmware/ESP32_SPEEDOMETER.ino](Firmware/ESP32_SPEEDOMETER.ino)* — Complete ESP32 Arduino firmware (GPS parsing, OLED UI, web server, and Telegram integration).
* *[ESP32_SPEEDOMETER_GERBERS_FILE.zip](ESP32_SPEEDOMETER_GERBERS_FILE.zip)* — Ready-to-manufacture Gerber and Excellon drill files.
* *[GPS MODULE.pdf](GPS%20MODULE.pdf)* — Printable circuit schematic diagram.
* *[GPS MODULE.kicad_sch](GPS%20MODULE.kicad_sch)* — Native KiCad 10 schematic source file.
* *[GPS MODULE_BILL_OF_MATERIAL.csv](GPS%20MODULE_BILL_OF_MATERIAL.csv)* — Exported KiCad Bill of Materials.

---

## Quick Setup & Flashing Guide
1. Open [Firmware/ESP32_SPEEDOMETER.ino](Firmware/ESP32_SPEEDOMETER.ino) in the *Arduino IDE*.
2. Install the required libraries via Library Manager: *TinyGPSPlus, *Adafruit GFX Library*, and *Adafruit SSD1306**.
3. Replace the placeholder Wi-Fi (WIFI_SSID, WIFI_PASS) and Telegram (BOT_TOKEN, CHAT_ID) credentials with your own.
4. Select *DOIT ESP32 DEVKIT V1* as your board and upload the firmware.
