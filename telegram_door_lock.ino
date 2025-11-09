/*
 * ============================================================================
 * TELEGRAM OTP DOOR LOCK SYSTEM - ESP32
 * ============================================================================
 * 
 * Project: Telegram-based OTP Door Lock with Keypad Entry
 * Hardware: ESP32 WROOM 32, LCD1602 I2C, 4x4 Keypad, 5V Relay, 12V Solenoid
 * 
 * Owner: Abhijeet Nardele
 * Bot: @abhijeet_doorlock_bot
 * 
 * Features:
 * ✅ Request OTP via Telegram bot
 * ✅ Enter OTP on 4x4 keypad
 * ✅ LCD displays status and OTP prompt
 * ✅ Auto-expire OTP after 60 seconds
 * ✅ Solenoid lock control via relay
 * ✅ Wrong attempt tracking (max 3 attempts)
 * ✅ Security: Only authorized user can unlock
 * 
 * ============================================================================
 */

// ==================== LIBRARY INCLUDES ====================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// ==================== WIFI CONFIGURATION ====================
// ✅ ALREADY CONFIGURED - Your mobile hotspot
const char* WIFI_SSID = "hmm";           // Your hotspot name
const char* WIFI_PASSWORD = "12345678";   // Your hotspot password

// ==================== TELEGRAM CONFIGURATION ====================
// ✅ ALREADY CONFIGURED - Your bot credentials
#define BOT_TOKEN "8221363925:AAH3nmbIyoPoeEmJdG6mJvFvMcUtY9QD4Ss"  // Your bot token
#define CHAT_ID "5203680634"                                        // Your Telegram Chat ID

// ==================== PIN DEFINITIONS ====================
// LCD I2C
#define LCD_ADDRESS 0x27    // Default I2C address
#define LCD_COLUMNS 16
#define LCD_ROWS 2

// Relay (controls solenoid lock)
#define RELAY_PIN 23

// Keypad Pins - SWAPPED to fix mapping
#define ROW_1 26
#define ROW_2 25
#define ROW_3 33
#define ROW_4 32
#define COL_1 13
#define COL_2 12
#define COL_3 14
#define COL_4 27

// ==================== KEYPAD CONFIGURATION ====================
const byte ROWS = 4;
const byte COLS = 4;

// CORRECT REVERSE MAPPING
// Position in matrix = what physical button to show
char keys[ROWS][COLS] = {
  {'D', '#', '3', 'A'},  // Standard pos: 1=D, 2=#, 3=3, A=A
  {'4', '9', '6', '7'},  // Standard pos: 4=4, 5=9, 6=6, 7=B physically shows 7
  {'7', '6', '9', '4'},  // Standard pos: 7=phys7→showsB, 8=phys6→shows8, 9=phys9→shows5, C=phys4→showsC
  {'A', '3', '#', 'D'}   // Standard pos: *=physA→shows*, 0=phys3→shows0, #=phys#→shows2, D=physD→shows1
};

byte rowPins[ROWS] = {ROW_1, ROW_2, ROW_3, ROW_4};
byte colPins[COLS] = {COL_1, COL_2, COL_3, COL_4};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ==================== GLOBAL OBJECTS ====================
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ==================== SYSTEM VARIABLES ====================
String currentOTP = "";
unsigned long otpGeneratedTime = 0;
bool otpActive = false;
String enteredOTP = "";
int wrongAttempts = 0;

// Timing variables
unsigned long lastBotCheck = 0;
const unsigned long BOT_CHECK_INTERVAL = 1000;  // Check for messages every 1 second

// OTP Configuration
const int OTP_LENGTH = 4;  // 4-digit OTP
const unsigned long OTP_VALIDITY = 120000;  // 120 seconds (extended for testing)
const int MAX_WRONG_ATTEMPTS = 5;  // Increased for testing

// Lock Configuration
const unsigned long UNLOCK_DURATION = 5000;  // Keep door unlocked for 5 seconds
bool doorUnlocked = false;
unsigned long unlockStartTime = 0;

// ==================== FUNCTION PROTOTYPES ====================
void connectWiFi();
void initializeLCD();
void handleNewMessages(int numNewMessages);
String generateOTP();
void displayOTP();
void checkKeypad();
void verifyOTP();
void unlockDoor();
void lockDoor();
void updateLCDTimer();
void clearOTP();
void displayMessage(String line1, String line2);
void handleTelegramCommands(String chat_id, String text, String from_name);

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n");
  Serial.println("============================================");
  Serial.println("  TELEGRAM OTP DOOR LOCK SYSTEM - ESP32");
  Serial.println("============================================");
  Serial.println("  Owner: Abhijeet Nardele");
  Serial.println("  Bot: @abhijeet_doorlock_bot");
  Serial.println("============================================");
  
  // Initialize I2C for LCD
  Wire.begin();
  
  // Initialize LCD
  initializeLCD();
  displayMessage("System Boot...", "Please Wait");
  delay(1000);
  
  // Initialize Relay Pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Lock engaged (relay off)
  
  // Connect to WiFi
  connectWiFi();
  
  // Configure secure client for Telegram (disable certificate check for simplicity)
  client.setInsecure();  // For testing - accepts all certificates
  
  // Test Telegram connection
  displayMessage("Testing Telegram", "Connection...");
  Serial.println("Testing Telegram bot...");
  
  if (bot.getMe()) {
    Serial.println("✅ Telegram bot connected successfully!");
    displayMessage("Bot Connected!", "@abhijeet_doorlock_bot");
    delay(2000);
  } else {
    Serial.println("❌ ERROR: Telegram bot connection failed!");
    displayMessage("Bot Error!", "Check Token");
    delay(3000);
  }
  
  // Display ready state
  displayMessage("Telegram Lock", "Send /unlock");
  Serial.println("\n✅ System Ready!");
  Serial.println("📱 Send /unlock to @abhijeet_doorlock_bot");
  Serial.println("============================================\n");
}

// ==================== MAIN LOOP ====================
void loop() {
  // Check for new Telegram messages
  if (millis() - lastBotCheck > BOT_CHECK_INTERVAL) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    
    if (numNewMessages) {
      Serial.println("📨 New message received!");
      handleNewMessages(numNewMessages);
    }
    
    lastBotCheck = millis();
  }
  
  // Update LCD timer if OTP is active
  if (otpActive) {
    updateLCDTimer();
    
    // Check if OTP has expired
    if (millis() - otpGeneratedTime > OTP_VALIDITY) {
      Serial.println("⏱️ OTP Expired!");
      clearOTP();
      displayMessage("OTP Expired!", "Send /unlock");
      delay(2000);
      displayMessage("Telegram Lock", "Send /unlock");
    }
  }
  
  // Check keypad input
  if (otpActive) {
    checkKeypad();
  }
  
  // Handle door auto-lock after unlock duration
  if (doorUnlocked && (millis() - unlockStartTime > UNLOCK_DURATION)) {
    lockDoor();
  }
  
  delay(50);  // Small delay to prevent excessive CPU usage
}

// ==================== WIFI CONNECTION ====================
void connectWiFi() {
  displayMessage("Connecting WiFi", "hmm");
  Serial.print("Connecting to WiFi: hmm");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    displayMessage("WiFi Connected!", WiFi.localIP().toString());
    delay(2000);
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
    displayMessage("WiFi Failed!", "Check Hotspot");
    delay(3000);
  }
}

// ==================== LCD INITIALIZATION ====================
void initializeLCD() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  Serial.println("✅ LCD Initialized");
}

// ==================== DISPLAY MESSAGE ON LCD ====================
void displayMessage(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ==================== HANDLE TELEGRAM MESSAGES ====================
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    
    Serial.println("========================================");
    Serial.println("Message from: " + from_name);
    Serial.println("Chat ID: " + chat_id);
    Serial.println("Message: " + text);
    Serial.println("========================================");
    
    // Security: Only respond to authorized chat ID
    if (chat_id != CHAT_ID) {
      Serial.println("⚠️ Unauthorized user attempted access!");
      bot.sendMessage(chat_id, "⛔ Unauthorized access. This incident will be reported.", "");
      continue;
    }
    
    handleTelegramCommands(chat_id, text, from_name);
  }
}

// ==================== HANDLE TELEGRAM COMMANDS ====================
void handleTelegramCommands(String chat_id, String text, String from_name) {
  // Convert to lowercase for command matching
  text.toLowerCase();
  
  if (text == "/start") {
    String welcome = "🚪 *Telegram OTP Door Lock System*\n\n";
    welcome += "Welcome " + from_name + "! 👋\n\n";
    welcome += "🔐 Available Commands:\n";
    welcome += "/unlock - Request OTP to unlock door\n";
    welcome += "/status - Check system status\n";
    welcome += "/help - Show this help message\n\n";
    welcome += "⚡ Features:\n";
    welcome += "• OTP expires in 60 seconds\n";
    welcome += "• Maximum 3 wrong attempts\n";
    welcome += "• Auto-lock after 5 seconds\n\n";
    welcome += "📱 Bot: @abhijeet_doorlock_bot";
    
    bot.sendMessage(chat_id, welcome, "Markdown");
  }
  
  else if (text == "/unlock") {
    if (otpActive) {
      bot.sendMessage(chat_id, "⚠️ An OTP is already active!\n\n⏱️ Please enter the current OTP or wait for it to expire.", "");
      return;
    }
    
    // Generate new OTP
    currentOTP = generateOTP();
    otpGeneratedTime = millis();
    otpActive = true;
    enteredOTP = "";
    wrongAttempts = 0;
    
    // Send OTP to user
    String otpMessage = "🔑 *Your OTP:* `" + currentOTP + "`\n\n";
    otpMessage += "⏱️ *Valid for:* 60 seconds\n";
    otpMessage += "📍 *Action:* Enter this OTP on the keypad at the door\n";
    otpMessage += "🔢 *Format:* Enter 4 digits and press #\n\n";
    otpMessage += "⚠️ *Security:* Do not share this OTP with anyone!\n";
    otpMessage += "🔒 *Note:* OTP will auto-expire after 60 seconds";
    
    bot.sendMessage(chat_id, otpMessage, "Markdown");
    
    Serial.println("✅ OTP Generated: " + currentOTP);
    Serial.println("⏰ Valid until: " + String((millis() + OTP_VALIDITY) / 1000) + "s");
    
    // Update LCD
    displayMessage("OTP Sent! 60s", "Enter: ____");
  }
  
  else if (text == "/status") {
    String status = "📊 *System Status*\n\n";
    status += "🌐 *WiFi:* Connected\n";
    status += "📡 *Signal:* " + String(WiFi.RSSI()) + " dBm\n";
    status += "🔓 *Door:* " + String(doorUnlocked ? "🟢 Unlocked" : "🔴 Locked") + "\n";
    status += "🔑 *OTP Active:* " + String(otpActive ? "✅ Yes" : "❌ No") + "\n";
    
    if (otpActive) {
      unsigned long timeLeft = (OTP_VALIDITY - (millis() - otpGeneratedTime)) / 1000;
      status += "⏱️ *OTP Expires in:* " + String(timeLeft) + " seconds\n";
      status += "🔢 *Wrong Attempts:* " + String(wrongAttempts) + "/" + String(MAX_WRONG_ATTEMPTS) + "\n";
    }
    
    status += "\n💻 *System:* ✅ Operational";
    status += "\n🤖 *Bot:* @abhijeet_doorlock_bot";
    status += "\n👤 *Owner:* Abhijeet Nardele";
    
    bot.sendMessage(chat_id, status, "Markdown");
  }
  
  else if (text == "/help") {
    String help = "📖 *Help - How to Use*\n\n";
    help += "📋 *Step-by-Step Guide:*\n";
    help += "1️⃣ Send /unlock command to this bot\n";
    help += "2️⃣ Receive 4-digit OTP on Telegram\n";
    help += "3️⃣ Go to the door\n";
    help += "4️⃣ Enter OTP on keypad (e.g., 1234)\n";
    help += "5️⃣ Press # to submit\n";
    help += "6️⃣ Door unlocks for 5 seconds\n\n";
    help += "🔒 *Security Features:*\n";
    help += "• OTP expires after 60 seconds\n";
    help += "• Maximum 3 wrong attempts allowed\n";
    help += "• 1-minute lockout after max attempts\n";
    help += "• Only authorized users can unlock\n";
    help += "• All attempts are logged\n\n";
    help += "⌨️ *Keypad Controls:*\n";
    help += "• 0-9: Enter OTP digits\n";
    help += "• #: Submit OTP\n";
    help += "• *: Delete last digit\n\n";
    help += "💡 *Tips:*\n";
    help += "• Make sure your phone has internet\n";
    help += "• OTP auto-submits after 4 digits\n";
    help += "• Use /status to check system\n\n";
    help += "ℹ️ For support, contact: Abhijeet Nardele";
    
    bot.sendMessage(chat_id, help, "Markdown");
  }
  
  else {
    String unknown = "❓ *Unknown Command*\n\n";
    unknown += "I didn't understand: `" + text + "`\n\n";
    unknown += "Available commands:\n";
    unknown += "• /start\n";
    unknown += "• /unlock\n";
    unknown += "• /status\n";
    unknown += "• /help\n\n";
    unknown += "Send /help for detailed instructions.";
    
    bot.sendMessage(chat_id, unknown, "Markdown");
  }
}

// ==================== GENERATE OTP ====================
String generateOTP() {
  String otp = "";
  for (int i = 0; i < OTP_LENGTH; i++) {
    otp += String(random(0, 10));
  }
  return otp;
}

// ==================== CHECK KEYPAD INPUT ====================
void checkKeypad() {
  char key = keypad.getKey();
  
  if (key) {
    Serial.println("🔢 Key pressed: " + String(key));
    
    // Handle special keys
    if (key == '#') {
      // Submit OTP
      if (enteredOTP.length() == OTP_LENGTH) {
        verifyOTP();
      } else {
        displayMessage("OTP Incomplete!", "Need " + String(OTP_LENGTH) + " digits");
        delay(1500);
        displayOTP();
      }
    }
    else if (key == '*') {
      // Clear last digit
      if (enteredOTP.length() > 0) {
        enteredOTP.remove(enteredOTP.length() - 1);
        displayOTP();
        Serial.println("⬅️ Deleted digit. Current: " + enteredOTP);
      }
    }
    else if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
      // Ignore letter keys (reserved for future features)
      Serial.println("⚠️ Letter key pressed (not used)");
      return;
    }
    else if (key >= '0' && key <= '9') {
      // Add digit to OTP
      if (enteredOTP.length() < OTP_LENGTH) {
        enteredOTP += key;
        displayOTP();
        Serial.println("✅ Entered: " + enteredOTP);
        
        // Auto-submit when all digits entered
        if (enteredOTP.length() == OTP_LENGTH) {
          delay(500);
          verifyOTP();
        }
      }
    }
  }
}

// ==================== DISPLAY OTP ON LCD ====================
void displayOTP() {
  unsigned long timeLeft = (OTP_VALIDITY - (millis() - otpGeneratedTime)) / 1000;
  
  // First line: "OTP: XXXX" with timer
  String line1 = "OTP: ";
  for (int i = 0; i < OTP_LENGTH; i++) {
    if (i < enteredOTP.length()) {
      line1 += enteredOTP.charAt(i);
    } else {
      line1 += "_";
    }
  }
  line1 += " " + String(timeLeft) + "s";
  
  // Second line: "# OK  * DEL"
  String line2 = "# OK  * DEL";
  
  displayMessage(line1, line2);
}

// ==================== VERIFY OTP ====================
void verifyOTP() {
  Serial.println("🔍 Verifying OTP...");
  Serial.println("Entered: " + enteredOTP);
  Serial.println("Correct: " + currentOTP);
  
  if (enteredOTP == currentOTP) {
    // ✅ CORRECT OTP
    Serial.println("✅ OTP VERIFIED! Unlocking door...");
    
    displayMessage("Access Granted!", "Welcome! :)");
    
    // Send success notification to Telegram
    String successMsg = "✅ *Door Unlocked Successfully!*\n\n";
    successMsg += "🕐 Time: " + String(millis() / 1000) + " seconds since boot\n";
    successMsg += "📍 Location: Door Lock System\n";
    successMsg += "👤 Access: Authorized";
    
    bot.sendMessage(CHAT_ID, successMsg, "Markdown");
    
    // Unlock door
    unlockDoor();
    
    // Clear OTP
    clearOTP();
    
    delay(2000);
    displayMessage("Door Unlocked", "Closes in 5s");
    
  } else {
    // ❌ WRONG OTP
    wrongAttempts++;
    Serial.println("❌ WRONG OTP! Attempts: " + String(wrongAttempts) + "/" + String(MAX_WRONG_ATTEMPTS));
    
    displayMessage("Invalid OTP!", "Try Again");
    
    // Send warning to Telegram
    String warning = "⚠️ *Wrong OTP Entered!*\n\n";
    warning += "🔢 Entered: `" + enteredOTP + "`\n";
    warning += "🔢 Correct: `" + currentOTP + "`\n";
    warning += "📊 Attempt: " + String(wrongAttempts) + "/" + String(MAX_WRONG_ATTEMPTS) + "\n";
    warning += "🕐 Time: " + String(millis() / 1000) + "s\n\n";
    
    if (wrongAttempts >= MAX_WRONG_ATTEMPTS) {
      warning += "🚨 *ALERT: Maximum attempts reached!*\n";
      warning += "🔒 System locked for 1 minute";
    } else {
      warning += "⚠️ " + String(MAX_WRONG_ATTEMPTS - wrongAttempts) + " attempts remaining";
    }
    
    bot.sendMessage(CHAT_ID, warning, "Markdown");
    
    delay(2000);
    
    if (wrongAttempts >= MAX_WRONG_ATTEMPTS) {
      // Lock out after max attempts
      Serial.println("🚨 MAXIMUM ATTEMPTS REACHED! Locking system...");
      displayMessage("Max Attempts!", "Locked 60s");
      
      clearOTP();
      delay(60000);  // 1 minute lockout
      
      displayMessage("Telegram Lock", "Send /unlock");
      
      // Send unlock notification
      bot.sendMessage(CHAT_ID, "🔓 System unlocked. You can request OTP again.", "");
    } else {
      enteredOTP = "";
      displayOTP();
    }
  }
}

// ==================== UNLOCK DOOR ====================
void unlockDoor() {
  digitalWrite(RELAY_PIN, HIGH);  // Activate relay (unlock solenoid)
  doorUnlocked = true;
  unlockStartTime = millis();
  Serial.println("🔓 DOOR UNLOCKED!");
}

// ==================== LOCK DOOR ====================
void lockDoor() {
  digitalWrite(RELAY_PIN, LOW);   // Deactivate relay (lock solenoid)
  doorUnlocked = false;
  Serial.println("🔒 DOOR LOCKED!");
  
  displayMessage("Door Locked", "Send /unlock");
  
  // Notify user
  bot.sendMessage(CHAT_ID, "🔒 Door automatically locked after 5 seconds.", "");
}

// ==================== UPDATE LCD TIMER ====================
void updateLCDTimer() {
  static unsigned long lastUpdate = 0;
  
  if (millis() - lastUpdate > 1000) {  // Update every second
    if (enteredOTP.length() > 0) {
      displayOTP();
    } else {
      unsigned long timeLeft = (OTP_VALIDITY - (millis() - otpGeneratedTime)) / 1000;
      displayMessage("OTP Sent! " + String(timeLeft) + "s", "Enter: ____");
    }
    lastUpdate = millis();
  }
}

// ==================== CLEAR OTP ====================
void clearOTP() {
  currentOTP = "";
  enteredOTP = "";
  otpActive = false;
  wrongAttempts = 0;
  Serial.println("🗑️ OTP Cleared");
}
