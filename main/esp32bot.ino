/*
 * ----------------------------------------------------------------------------
 * Project: ESP32 Discord bot
 * Author:  PingVortex
 * Website: https://www.pingvortex.com
 * Source:  https://github.com/pingvortex/ESP32-Discord-bot

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

#define WEBSOCKETS_NETWORK_TYPE NETWORK_ESP32#include <WebSockets2_Generic.h>

#include <ArduinoJson.h>

#include "config.h"

#ifdef __cplusplus
extern "C" {
  #endif
  uint8_t temprature_sens_read();
  #ifdef __cplusplus
}
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, & Wire, -1);

using namespace websockets2_generic;

WebsocketsClient client;

unsigned long lastHeartbeat = 0;
unsigned long heartbeatInterval = 40000;
bool isConnected = false;

DynamicJsonDocument doc(3072);

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
  StaticJsonDocument < 512 > idDoc;
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

long sendMessage(const char * channelId, String content) {
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

void sendEmbed(const char * channelId, String title, String description, uint32_t color) {
  WiFiClientSecure httpsClient;
  httpsClient.setInsecure();
  HTTPClient http;
  String url = "https://discord.com/api/v10/channels/" + String(channelId) + "/messages";

  if (http.begin(httpsClient, url)) {
    http.addHeader("Authorization", "Bot " + String(token));
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument < 1024 > embedDoc;
    JsonArray embeds = embedDoc.createNestedArray("embeds");
    JsonObject embed = embeds.createNestedObject();

    embed["title"] = title;
    embed["description"] = description;
    embed["color"] = color;

    String payload;
    serializeJson(embedDoc, payload);

    http.POST(payload);
    http.end();
  }
}

void onMessageCallback(WebsocketsMessage message) {
  if (message.length() > 2500) return;

  doc.clear();
  DeserializationError error = deserializeJson(doc, message.data());
  if (error) return;

  if (doc["t"] == "MESSAGE_CREATE") {
    const char * content = doc["d"]["content"];
    const char * channelId = doc["d"]["channel_id"];
    bool isBot = doc["d"]["author"]["bot"] | false;

    if (content && !isBot) {
      String msg = String(content);

      // --->:test ---
      if (msg.indexOf(">:test") >= 0) {
        updateDisplay("CMD: Test");

        unsigned long totalSeconds = millis() / 1000;
        unsigned long days = totalSeconds / 86400;
        unsigned long hours = (totalSeconds % 86400) / 3600;
        unsigned long minutes = (totalSeconds % 3600) / 60;
        unsigned long seconds = totalSeconds % 60;

        // temp
        float temp_c = (temprature_sens_read() - 32) / 1.8;

        // memory usage
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t totalHeap = ESP.getHeapSize();
        float usagePercent = 100.0 * (1.0 - ((float) freeHeap / (float) totalHeap));

        String runtime = String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";

        // send normal message
        long latency = sendMessage(channelId, "im working :scream:");

        // send embed
        String embedDesc = "**System Info:**\n";
        embedDesc += "**Ping:** " + String(latency) + "ms\n";
        embedDesc += "**Memory Usage:** " + String(usagePercent, 1) + "% (" + String(freeHeap / 1024) + " KB free)\n";
        embedDesc += "**Internal Temp:** " + String(temp_c, 1) + " °C\n";
        embedDesc += "**Uptime:** " + runtime;

        sendEmbed(channelId, "Bot Status", embedDesc, 5763719);
      }

      // --->:ping ---
      else if (msg.indexOf(">:ping") >= 0) {
        updateDisplay("CMD: Pinging...");
        long latency = sendMessage(channelId, "im a slow clanka pls wait...");

        String result = "**Latency:** " + String(latency) + "ms";
        sendEmbed(channelId, "Pong!", result, 16711680);
        updateDisplay(String(latency) + "ms");
      }

      // --->:echo ---
      else if (msg.startsWith(">:echo ")) {
        int firstSpace = msg.indexOf(' ', 7);
        if (firstSpace != -1) {
          String targetChannel = msg.substring(7, firstSpace);
          String echoContent = msg.substring(firstSpace + 1);

          updateDisplay("Echoing...", targetChannel);
          sendMessage(targetChannel.c_str(), echoContent);

          sendMessage(channelId, "Message sent to <#" + targetChannel + ">");
        } else {
          sendMessage(channelId, "Usage: `>:echo <channel_id> <message>`");
        }
      }

      // --->:flip ---
      else if (msg.indexOf(">:flip") >= 0) {
        String side = (random(0, 2) == 0) ? "Heads" : "Tails";
        sendMessage(channelId, "Result: " + side);
        updateDisplay("Flipped: " + side);
      }

      // --->:rng ---
      else if (msg.startsWith(">:rng ")) {
        int spaceIndex = msg.indexOf(' ', 6);
        if (spaceIndex != -1) {
          int minVal = msg.substring(6, spaceIndex).toInt();
          int maxVal = msg.substring(spaceIndex + 1).toInt();

          if (maxVal > minVal) {
            long result = random(minVal, maxVal + 1);
            updateDisplay("RNG Result", String(result));
            sendMessage(channelId, "Rolled (" + String(minVal) + "-" + String(maxVal) + "): **" + String(result) + "**");
          } else {
            sendMessage(channelId, "Max must be greater than min!");
          }
        } else {
          sendMessage(channelId, "Usage: >:rng <min> <max>");
        }
      }
    }
  }

  int op = doc["op"] | -1;
  if (op == 10) {
    heartbeatInterval = doc["d"]["heartbeat_interval"];
    identify();
    isConnected = true;
    updateDisplay("Online");
  }
}

void setup() {
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  updateDisplay("Connecting to WiFi...");
  WiFi.begin(wifiName, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  updateDisplay("Connecting to Discord...");
  client.onMessage(onMessageCallback);
  client.setInsecure();
  client.connect("wss://gateway.discord.gg/?v=10&encoding=json");
}

void loop() {

  // handle disconnects
  if (!client.available()) {
    isConnected = false;
    updateDisplay("Reconnecting...");
    client.connect("wss://gateway.discord.gg/?v=10&encoding=json");
    delay(2000); // prevent spamming 
    return;
  }

  client.poll();

  if (isConnected && (millis() - lastHeartbeat > heartbeatInterval)) {
    StaticJsonDocument < 128 > hb;
    hb["op"] = 1;
    hb["d"] = nullptr;
    String hbOut;
    serializeJson(hb, hbOut);
    client.send(hbOut);
    lastHeartbeat = millis();
  }
}
