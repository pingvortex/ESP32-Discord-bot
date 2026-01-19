/*
 * ----------------------------------------------------------------------------
 * Project: ESP32 Discord bot host
 * Author:  PingVortex
 * Website: https://www.pingvortex.com
 * Source:  https://github.com/pingvortex/ESP32-Discord-bot-host

 * * License:  Creative Commons Attribution-NonCommercial-ShareAlike 4.0 
 * (CC BY-NC-SA 4.0)

 * * Description:
 * A Discord bot running natively on an ESP32 using WebSockets. 
 * Displays status updates on an SSD1306 OLED screen.

 * * Hardware:
 * - ESP32
 * - SSD1306 128x64 I2C OLED (NOT REQUIRED)
 * ----------------------------------------------------------------------------
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define WEBSOCKETS_NETWORK_TYPE NETWORK_ESP32
#include <WebSockets2_Generic.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

using namespace websockets2_generic;

WebsocketsClient client;

unsigned long lastHeartbeat = 0;
unsigned long heartbeatInterval = 40000;
bool isConnected = false;

DynamicJsonDocument doc(3072);

  // --- SETTINGS ---
  const char* token = "YOUR_BOT_TOKEN"; // YOUR BOT TOKEN

  String wifiName = "SSID"; // YOUR WIFI NAME/SSID
  String wifiPassword = "PASSWORD"; // YOUR WIFI PASSWORD
  String botName = "BOT"; // WHAT NAME DO YOU WANT TO BE DISPLAYED ON THE DISPLAY (Can be left as BOT) 

void updateDisplay(String status, String lastMsg = "") {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(botName);
    display.println("Status: " + status);
    display.println("---------------------");
    if (lastMsg != "") {
        display.print("> ");
        display.println(lastMsg);
    }
    display.display();
}

void identify() {
    StaticJsonDocument<512> idDoc;
    idDoc["op"] = 2;
    JsonObject d = idDoc.createNestedObject("d");
    d["token"] = token;
    
    JsonObject prop = d.createNestedObject("properties");
    prop["$os"] = "esp32";
    prop["$browser"] = "esp32-bot";
    prop["$device"] = "esp32";

    // 33280 = GUILD_MESSAGES (512) + MESSAGE_CONTENT
    d["intents"] = 33280; 

    String output;
    serializeJson(idDoc, output);
    client.send(output);
}

long sendMessage(const char* channelId, String content) {
    long startSend = millis();
    WiFiClientSecure httpsClient;
    httpsClient.setInsecure();
    HTTPClient http;
    String url = "https://discord.com/api/v10/channels/" + String(channelId) + "/messages";
    
    if (http.begin(httpsClient, url)) {
        http.addHeader("Authorization", "Bot " + String(token));
        http.addHeader("Content-Type", "application/json");
        String payload = "{\"content\":\"" + content + "\"}";
        int httpCode = http.POST(payload);
        http.end();
    }
    return millis() - startSend;
}

void onMessageCallback(WebsocketsMessage message) {
    if (message.length() > 2500) return;

    doc.clear();
    DeserializationError error = deserializeJson(doc, message.data());
    if (error) return;

    if (doc["t"] == "MESSAGE_CREATE") {
        const char* content = doc["d"]["content"];
        const char* channelId = doc["d"]["channel_id"];
        bool isBot = doc["d"]["author"]["bot"] | false;

        if (content && !isBot) {
            String msg = String(content);
            
            // --- >:test ---
            if (msg.indexOf(">:test") >= 0) {
                updateDisplay("CMD: test");
                sendMessage(channelId, "im working :scream:");
            }
            
            // --- >:ping ---
            else if (msg.indexOf(">:ping") >= 0) {
                updateDisplay("CMD: pinging...");
                long latency = sendMessage(channelId, "im a slow clanka pls wait...");
                
                String result = "my clanka latency: " + String(latency) + "ms";
                sendMessage(channelId, result);
                updateDisplay(String(latency) + "ms");
            }

          // --- >:say ---
            else if (msg.startsWith(">:say ")) {
                String note = msg.substring(6);
                updateDisplay("DISCORD MSG", note);
                sendMessage(channelId, note);
            }

          // --- >:flip ---
            else if (msg.indexOf(">:flip") >= 0) {
                String side = (random(0, 2) == 0) ? "Heads" : "Tails";
                sendMessage(channelId, "Result: " + side);
                updateDisplay("flipped: " + side);
            }

        }
    }

        int op = doc["op"] | -1;
    if (op == 10) { 
        heartbeatInterval = doc["d"]["heartbeat_interval"];
        identify();
        isConnected = true;
        updateDisplay("online");
    }
}

void setup() {
    Serial.begin(115200);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    
    updateDisplay("connecting to wifi");
    WiFi.begin(wifiName.c_str(), wifiPassword.c_str());
    while (WiFi.status() != WL_CONNECTED) delay(500);

    updateDisplay("connecting to discord");
    client.onMessage(onMessageCallback);
    client.setInsecure(); 
    client.connect("wss://gateway.discord.gg/?v=10&encoding=json");
}

void loop() {
    client.poll();

    if (isConnected && (millis() - lastHeartbeat > heartbeatInterval)) {
        StaticJsonDocument<128> hb;
        hb["op"] = 1;
        hb["d"] = nullptr; 
        String hbOut;
        serializeJson(hb, hbOut);
        client.send(hbOut);
        lastHeartbeat = millis();
    }
}
