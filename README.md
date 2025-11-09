# Telegram OTP Door Lock System 🔐

IoT door lock system using ESP32 and Telegram bot for secure OTP-based access control with 4x4 keypad and LCD display.

## 📋 Project Overview

This project demonstrates an intelligent door lock system that combines IoT technology with secure access control. Built for Logic & Microprocessor coursework, it showcases real-world application of microcontroller programming, IoT protocols, and security concepts.

## ✨ Features

- 🔑 **OTP-Based Security**: Generate one-time passwords via Telegram bot
- ⌨️ **4x4 Keypad Entry**: Physical keypad for OTP input
- 📱 **Telegram Integration**: Control lock remotely through @abhijeet_doorlock_bot
- 📺 **LCD Display**: Real-time status updates on 16x2 I2C LCD
- ⏱️ **Auto-Expiry**: OTP expires after 120 seconds
- 🔒 **Security Features**: 
  - Maximum 5 wrong attempts before lockout
  - Authorized user verification
  - Relay-controlled solenoid lock
- 🚪 **Automatic Locking**: Door auto-locks after 5 seconds

## 🛠️ Hardware Requirements

| Component | Specification | Quantity |
|-----------|--------------|----------|
| ESP32 | WROOM 32 | 1 |
| LCD Display | 16x2 I2C (Address: 0x27) | 1 |
| Keypad | 4x4 Matrix Keypad | 1 |
| Relay Module | 5V Single Channel | 1 |
| Solenoid Lock | 12V DC | 1 |
| Power Supply | 12V DC Adapter | 1 |
| Jumper Wires | Male-to-Female | As needed |

## 📐 Pin Configuration

### ESP32 Pin Connections

**LCD I2C:**
- SDA: GPIO 21 (default)
- SCL: GPIO 22 (default)
- VCC: 3.3V
- GND: GND

**Relay Module:**
- Signal: GPIO 23
- VCC: 5V
- GND: GND

**4x4 Keypad:**
- ROW 1: GPIO 26
- ROW 2: GPIO 25
- ROW 3: GPIO 33
- ROW 4: GPIO 32
- COL 1: GPIO 13
- COL 2: GPIO 12
- COL 3: GPIO 14
- COL 4: GPIO 27

## 📚 Required Libraries

Install these libraries via Arduino IDE Library Manager:

```
- WiFi.h (ESP32 Core)
- WiFiClientSecure.h (ESP32 Core)
- UniversalTelegramBot by Brian Lough
- ArduinoJson by Benoit Blanchon (v6.x)
- Wire.h (ESP32 Core)
- LiquidCrystal_I2C by Frank de Brabander
- Keypad by Mark Stanley
```

## ⚙️ Setup Instructions

### 1. Hardware Assembly
1. Connect LCD to ESP32 via I2C (SDA/SCL)
2. Wire 4x4 keypad according to pin configuration
3. Connect relay module to GPIO 23
4. Connect solenoid lock to relay (12V supply)
5. Power ESP32 via USB or external 5V supply

### 2. Telegram Bot Setup
1. Open Telegram and search for [@BotFather](https://t.me/BotFather)
2. Create new bot: `/newbot`
3. Save your Bot Token
4. Get your Chat ID from [@userinfobot](https://t.me/userinfobot)

### 3. Software Configuration
1. Open `telegram_door_lock.ino` in Arduino IDE
2. Update WiFi credentials:
   ```cpp
   const char* WIFI_SSID = "YourWiFiName";
   const char* WIFI_PASSWORD = "YourPassword";
   ```
3. Update Telegram credentials:
   ```cpp
   #define BOT_TOKEN "YOUR_BOT_TOKEN"
   #define CHAT_ID "YOUR_CHAT_ID"
   ```
4. Select board: **ESP32 Dev Module**
5. Upload code to ESP32

## 🚀 Usage

### Unlocking the Door

1. **Request OTP:**
   - Open Telegram
   - Message your bot: `/unlock`
   - Receive 4-digit OTP

2. **Enter OTP:**
   - Type OTP on the 4x4 keypad
   - Press `#` to submit
   - Door unlocks for 5 seconds on correct OTP

3. **Status Display:**
   - LCD shows OTP validity timer
   - Wrong attempts are tracked
   - Auto-locks after timeout

### Telegram Commands

| Command | Description |
|---------|-------------|
| `/start` | Welcome message and instructions |
| `/unlock` | Generate new OTP for door unlock |
| `/status` | Check system status |

## 🔧 Configuration Parameters

You can modify these in the code:

```cpp
const int OTP_LENGTH = 4;                    // OTP digit count
const unsigned long OTP_VALIDITY = 120000;   // OTP timeout (ms)
const int MAX_WRONG_ATTEMPTS = 5;            // Max wrong attempts
const unsigned long UNLOCK_DURATION = 5000;  // Unlock time (ms)
```

## 🎓 Academic Context

**Course:** Logic & Microprocessor  
**Owner:** Abhijeet Nardele  
**Institution:** [Your College/University Name]

This project demonstrates:
- Microcontroller interfacing (ESP32)
- I2C communication protocol
- GPIO programming
- IoT integration (WiFi, Telegram API)
- Real-time systems and timing
- Security implementation
- Human-machine interface design

## 🐛 Troubleshooting

**WiFi Connection Issues:**
- Verify SSID and password
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check WiFi signal strength

**LCD Not Displaying:**
- Verify I2C address (use I2C scanner sketch)
- Check SDA/SCL connections
- Adjust LCD contrast potentiometer

**Telegram Bot Not Responding:**
- Verify Bot Token and Chat ID
- Check internet connection
- Ensure bot is not blocked

**Keypad Mapping Issues:**
- Current mapping is custom-configured
- Test with keypad test sketch if needed
- Verify all row/column connections

## 📝 License

This project is open-source and available for educational purposes.

## 👤 Author

**Abhijeet Nardele**
- GitHub: [@abhijeetnardele24-hash](https://github.com/abhijeetnardele24-hash)
- Telegram Bot: [@abhijeet_doorlock_bot](https://t.me/abhijeet_doorlock_bot)

## 🙏 Acknowledgments

- UniversalTelegramBot library by Brian Lough
- ESP32 community and documentation
- Arduino IDE and libraries

---

⭐ If you find this project useful, please consider giving it a star!

**Note:** Remember to remove or replace sensitive credentials (WiFi password, Bot Token, Chat ID) before sharing your code publicly.
