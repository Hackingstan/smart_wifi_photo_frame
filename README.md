# 📸 ESP8266 Smart WiFi Photo Frame

A high-performance, smart digital photo frame using an ESP8266 (NodeMCU) and a 240x320 ST7789 SPI TFT display. This project features a built-in Smart Web-App that runs on your phone's browser, automatically cropping and resizing images before sending them to the display. This ensures zero lag and prevents the ESP8266 from running out of memory!

## ✨ Features
- **Smart Web UI:** Upload photos directly from your phone/PC browser.
- **Client-Side Auto Resizing:** The web app resizes heavy images (4K/HD) to exactly 240x320 resolution using HTML5 Canvas before uploading.
- **Real-Time Brightness Control:** Adjust screen brightness via a web-based slider (PWM).
- **Fast Image Rendering:** Uses the highly optimized `JPEGDEC` library to decode and render images quickly from the SD Card.

## 🛠 Hardware Required
- ESP8266 (NodeMCU)
- 240x320 SPI TFT Display (ST7789 Driver)
- MicroSD Card Module & MicroSD Card
- Jumper Wires

## 🔌 Wiring Diagram (Pinout)
| Component | ESP8266 Pin | Notes |
| :--- | :--- | :--- |
| **Display VCC** | Vin / 5V | Power for display |
| **Display GND** | GND | Ground |
| **Display BLK** | D3 (GPIO 0) | Brightness Control (PWM) |
| **Display CS** | D1 (GPIO 5) | Chip Select |
| **Display DC/RS** | D2 (GPIO 4) | Data/Command |
| **Display RES** | 3.3V | Reset |
| **Display SDA** | D7 (GPIO 13) | MOSI |
| **Display SCL** | D5 (GPIO 14) | SCK |
| **SD_CS** | D8 (GPIO 15) | SD Card Chip Select |
| **SD_MOSI** | D7 (GPIO 13) | Shared with Display |
| **SD_SCK** | D5 (GPIO 14) | Shared with Display |
| **SD_MISO** | D6 (GPIO 12) | **Only** SD Card MISO |

*Note: Do NOT connect the Display's SDO (MISO) pin to avoid SPI noise/collision.*

## 📚 Libraries Required
Install the following libraries via the Arduino IDE Library Manager:
1. `Adafruit GFX Library`
2. `Adafruit ST7735 and ST7789 Library`
3. `JPEGDEC` by Larry Bank

## 🚀 How to Use
1. Format your MicroSD card to FAT32 and insert it into the module.
2. Update the WiFi `AP_SSID` and `AP_PASS` in the code if needed.
3. Upload the code to your ESP8266.
4. Connect your phone/PC to the "PHOTO-FRAME" WiFi network.
5. Open a web browser and go to the IP address displayed on the TFT screen.
6. Upload a photo and use the brightness slider!
