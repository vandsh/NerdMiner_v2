#include "displayDriver.h"

#ifdef HEADLESS_DISPLAY

#include <Arduino.h>
#include <WiFi.h>
#include "monitor.h"
#include "wManager.h"

#ifdef ENABLE_WEB_STATS
#include "../../webServer.h"
#endif

extern monitor_data mMonitor;
bool ledOn = true;
unsigned long previousMillis = 0;
bool webServerInitialized = false;

void headless_Init(void)
{
    Serial.println("==================================");
    Serial.println("NerdMiner v2 - Headless Mode");
    Serial.println("==================================");
    Serial.println("Initializing headless display driver...");
    pinMode(LED_PIN, OUTPUT);
    
    Serial.print("LED Pin: ");
    Serial.println(LED_PIN);
    Serial.println("Display output redirected to Serial");
}

void headless_AlternateScreenState(void)
{
    Serial.println("[Display] Toggling LED state");
    ledOn = !ledOn;
}

void headless_AlternateRotation(void)
{
    // No rotation in headless mode
}

void headless_NoScreen(unsigned long mElapsed)
{
    // Check if we should initialize the web server
    #ifdef ENABLE_WEB_STATS
    if (!webServerInitialized && WiFi.status() == WL_CONNECTED) {
        setupWebServer();
        webServerInitialized = true;
        
        Serial.println("");
        Serial.println("==================================");
        Serial.println("Web Server Ready!");
        Serial.println("==================================");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Access stats at: http://");
        Serial.print(WiFi.localIP());
        Serial.println("/");
        Serial.println("==================================");
    }
    #endif
    
    // Log mining status periodically (every 30 seconds)
    static unsigned long lastStatusLog = 0;
    if (millis() - lastStatusLog >= 30000) {
        lastStatusLog = millis();
        
        Serial.println("");
        Serial.println("--- Mining Status ---");
        Serial.print("Status: ");
        switch (mMonitor.NerdStatus) {
            case NM_waitingConfig:
                Serial.println("Waiting for Configuration");
                break;
            case NM_Connecting:
                Serial.println("Connecting to Pool");
                break;
            case NM_hashing:
                Serial.println("Mining / Hashing");
                break;
            default:
                Serial.println("Unknown");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("WiFi: Connected (");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm)");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("WiFi: Disconnected");
        }
        
        Serial.print("Temperature: ");
        Serial.print(temperatureRead());
        Serial.println("°C");
        Serial.print("Free Heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
        Serial.println("--------------------");
    }
}

void headless_LoadingScreen(void)
{
    Serial.println("[Display] Loading...");
    Serial.println("Please wait while NerdMiner initializes");
}

void headless_SetupScreen(void)
{
    Serial.println("[Display] Setup Mode");
    Serial.println("Waiting for configuration...");
    Serial.println("Connect to the WiFi AP to configure settings");
}

void headless_DoLedStuff(unsigned long frame)
{
    unsigned long currentMillis = millis();

    if (!ledOn)
    {
        digitalWrite(LED_PIN, INACTIVE_LED);
        return;
    }

    switch (mMonitor.NerdStatus)
    {
        case NM_waitingConfig:
            // LED on continuously
            digitalWrite(LED_PIN, ACTIVE_LED);
            break;

        case NM_Connecting:
            // 0.5sec blink
            if (currentMillis - previousMillis >= 500)
            {
                previousMillis = currentMillis;
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            }
            break;

        case NM_hashing:
            // 0.1sec blink
            if (currentMillis - previousMillis >= 100)
            {
                previousMillis = currentMillis;
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            }
            break;
    }
}

void headless_AnimateCurrentScreen(unsigned long frame)
{
    // No animation in headless mode
}

CyclicScreenFunction headlessCyclicScreens[] = {headless_NoScreen};

DisplayDriver headlessDisplayDriver = {
    headless_Init,
    headless_AlternateScreenState,
    headless_AlternateRotation,
    headless_LoadingScreen,
    headless_SetupScreen,
    headlessCyclicScreens,
    headless_AnimateCurrentScreen,
    headless_DoLedStuff,
    SCREENS_ARRAY_SIZE(headlessCyclicScreens),
    0,
    0,
    0,
};

#endif // HEADLESS_DISPLAY
