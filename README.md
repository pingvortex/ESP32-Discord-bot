# ESP32 Discord Bot Host

A native Discord bot running on an ESP32 using WebSockets. This project allows you to host a functional Discord bot on a microcontroller without needing a 24/7 PC or a VPS. It also features optional real-time status updates on an SSD1306 OLED display.

> [!NOTE] 
> This project has been developed and tested specifically on the ESP32 DevKit V1. While it may work on other ESP32 variants, pin layouts and compatibility have not been verified.

## Features

* **Native WebSockets**: Connects directly to the Discord Gateway (v10).
* **Command Handling**: Supports basic interactive commands.
* **OLED Integration**: Displays bot status, connectivity, and command logs on a 128x64 I2C screen.
* **Low Power**: Efficiently handles heartbeats and message parsing via ArduinoJson.

## Hardware Requirements

* **ESP32**
* **SSD1306 128x64 I2C OLED** (Optional)

## Bot Commands

* `>:test` - Checks if the bot is responding and see some stats.
* `>:ping` - Measures the latency of the ESP32.
* `>:say <message>` - Makes the bot echo a text you say.
* `>:flip` - Flips a coin.

## Setup Instructions

1. **Discord Developer Portal**:
* Create a new application and get your **Bot Token**.
* Enable **Message Content Intent** in the "Bot" settings tab.


2. **Configuration**:
* Open the `config.h` file and update the following variables:
* `BOT_TOKEN`: Your Discord Bot Token.
* `WIFI_SSID`: Your WiFi SSID.
* `WIFI_PASSWORD`: Your WiFi Password.
* `BOT_NAME`: What name do you want displayed on the OLED.




3. **Wiring**:
* VCC -> 3.3V
* GND -> GND
* SCL -> GPIO 22
* SDA -> GPIO 21


4. **Flash**: Select your ESP32 board in Arduino IDE and upload the sketch.

---

### License

**Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0)**
