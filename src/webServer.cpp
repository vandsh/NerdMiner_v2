#include "webServer.h"

#ifdef ENABLE_WEB_STATS

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "monitor.h"
#include "mining.h"
#include "drivers/storage/storage.h"

// External variables from mining.cpp
extern uint32_t templates;
extern uint32_t hashes;
extern uint32_t Mhashes;
extern uint32_t totalKHashes;
extern uint32_t elapsedKHs;
extern uint64_t upTime;
extern uint32_t shares;
extern uint32_t valids;
extern double best_diff;

// External variables from monitor.cpp
extern monitor_data mMonitor;
extern TSettings Settings;

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NerdMiner Stats</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #ffffff;
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        h1 {
            text-align: center;
            margin-bottom: 30px;
            font-size: 2.5em;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        .stat-card {
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            border-radius: 15px;
            padding: 20px;
            box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);
            border: 1px solid rgba(255, 255, 255, 0.18);
            transition: transform 0.3s ease;
        }
        .stat-card:hover {
            transform: translateY(-5px);
        }
        .stat-label {
            font-size: 0.9em;
            opacity: 0.8;
            margin-bottom: 8px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .stat-value {
            font-size: 2em;
            font-weight: bold;
            color: #4CAF50;
        }
        .stat-value.warning {
            color: #ff9800;
        }
        .stat-value.error {
            color: #f44336;
        }
        .info-section {
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            border-radius: 15px;
            padding: 25px;
            margin-bottom: 20px;
            box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);
            border: 1px solid rgba(255, 255, 255, 0.18);
        }
        .info-row {
            display: flex;
            justify-content: space-between;
            padding: 12px 0;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
        }
        .info-row:last-child {
            border-bottom: none;
        }
        .info-label {
            font-weight: 600;
            opacity: 0.9;
        }
        .info-value {
            color: #4CAF50;
        }
        .button-container {
            text-align: center;
            margin-top: 20px;
        }
        .refresh-btn {
            background: linear-gradient(135deg, #4CAF50 0%, #45a049 100%);
            color: white;
            border: none;
            padding: 15px 40px;
            font-size: 1.1em;
            border-radius: 50px;
            cursor: pointer;
            box-shadow: 0 4px 15px rgba(76, 175, 80, 0.3);
            transition: all 0.3s ease;
        }
        .refresh-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(76, 175, 80, 0.4);
        }
        .refresh-btn:active {
            transform: translateY(0);
        }
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
            animation: pulse 2s infinite;
        }
        .status-mining {
            background-color: #4CAF50;
        }
        .status-connecting {
            background-color: #ff9800;
        }
        .status-waiting {
            background-color: #f44336;
        }
        @keyframes pulse {
            0%, 100% {
                opacity: 1;
            }
            50% {
                opacity: 0.5;
            }
        }
        @media (max-width: 768px) {
            h1 {
                font-size: 2em;
            }
            .stat-value {
                font-size: 1.5em;
            }
            .stats-grid {
                grid-template-columns: 1fr;
            }
        }
        .last-update {
            text-align: center;
            margin-top: 20px;
            opacity: 0.7;
            font-size: 0.9em;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>⛏️ NerdMiner v2 Stats</h1>
        
        <div class="stats-grid">
            <div class="stat-card">
                <div class="stat-label">Hash Rate</div>
                <div class="stat-value" id="hashrate">--</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Valid Shares</div>
                <div class="stat-value" id="valids">--</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Total Shares</div>
                <div class="stat-value" id="shares">--</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Temperature</div>
                <div class="stat-value" id="temperature">--</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Uptime</div>
                <div class="stat-value" id="uptime">--</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Best Difficulty</div>
                <div class="stat-value" id="bestDiff">--</div>
            </div>
        </div>

        <div class="info-section">
            <h2 style="margin-bottom: 20px;">Mining Information</h2>
            <div class="info-row">
                <span class="info-label">Status:</span>
                <span class="info-value"><span class="status-indicator" id="statusIndicator"></span><span id="status">--</span></span>
            </div>
            <div class="info-row">
                <span class="info-label">Pool URL:</span>
                <span class="info-value" id="poolUrl">--</span>
            </div>
            <div class="info-row">
                <span class="info-label">Pool Port:</span>
                <span class="info-value" id="poolPort">--</span>
            </div>
            <div class="info-row">
                <span class="info-label">BTC Address:</span>
                <span class="info-value" id="btcAddress" style="word-break: break-all;">--</span>
            </div>
            <div class="info-row">
                <span class="info-label">Total KHashes:</span>
                <span class="info-value" id="totalKHashes">--</span>
            </div>
            <div class="info-row">
                <span class="info-label">Templates:</span>
                <span class="info-value" id="templates">--</span>
            </div>
            <div class="info-row">
                <span class="info-label">Free Heap:</span>
                <span class="info-value" id="freeHeap">--</span>
            </div>
            <div class="info-row">
                <span class="info-label">WiFi Signal:</span>
                <span class="info-value" id="wifiSignal">--</span>
            </div>
        </div>

        <div class="button-container">
            <button class="refresh-btn" onclick="updateStats()">🔄 Refresh Now</button>
        </div>
        
        <div class="last-update">
            Last updated: <span id="lastUpdate">Never</span>
        </div>
    </div>

    <script>
        function updateStats() {
            fetch('/api/stats')
                .then(response => response.json())
                .then(data => {
                    // Update main stats
                    document.getElementById('hashrate').textContent = data.hashRate;
                    document.getElementById('valids').textContent = data.valids;
                    document.getElementById('shares').textContent = data.shares;
                    document.getElementById('temperature').textContent = data.temperature + '°C';
                    document.getElementById('uptime').textContent = data.uptime;
                    document.getElementById('bestDiff').textContent = data.bestDiff;
                    
                    // Update info section
                    document.getElementById('status').textContent = data.status;
                    document.getElementById('poolUrl').textContent = data.poolUrl;
                    document.getElementById('poolPort').textContent = data.poolPort;
                    document.getElementById('btcAddress').textContent = data.btcAddress;
                    document.getElementById('totalKHashes').textContent = data.totalKHashes;
                    document.getElementById('templates').textContent = data.templates;
                    document.getElementById('freeHeap').textContent = data.freeHeap + ' bytes';
                    document.getElementById('wifiSignal').textContent = data.wifiSignal + ' dBm';
                    
                    // Update status indicator
                    const indicator = document.getElementById('statusIndicator');
                    indicator.className = 'status-indicator';
                    if (data.status === 'Mining') {
                        indicator.classList.add('status-mining');
                    } else if (data.status === 'Connecting') {
                        indicator.classList.add('status-connecting');
                    } else {
                        indicator.classList.add('status-waiting');
                    }
                    
                    // Update last update time
                    document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();
                })
                .catch(error => {
                    console.error('Error fetching stats:', error);
                });
        }

        // Update stats on page load
        updateStats();
        
        // Auto-refresh every 5 seconds
        setInterval(updateStats, 5000);
    </script>
</body>
</html>
)rawliteral";

void handleStatsAPI(AsyncWebServerRequest *request) {
    StaticJsonDocument<1024> doc;
    
    // Calculate hash rate
    unsigned long currentKHashes = (Mhashes * 1000) + hashes / 1000;
    double hashRate = (currentKHashes - elapsedKHs) / (upTime / 1000.0);
    char hashRateStr[20];
    sprintf(hashRateStr, "%.2f KH/s", hashRate);
    
    // Calculate uptime
    unsigned long uptimeSeconds = upTime / 1000;
    unsigned long days = uptimeSeconds / 86400;
    unsigned long hours = (uptimeSeconds % 86400) / 3600;
    unsigned long minutes = (uptimeSeconds % 3600) / 60;
    unsigned long seconds = uptimeSeconds % 60;
    char uptimeStr[50];
    if (days > 0) {
        sprintf(uptimeStr, "%lud %luh %lum %lus", days, hours, minutes, seconds);
    } else if (hours > 0) {
        sprintf(uptimeStr, "%luh %lum %lus", hours, minutes, seconds);
    } else if (minutes > 0) {
        sprintf(uptimeStr, "%lum %lus", minutes, seconds);
    } else {
        sprintf(uptimeStr, "%lus", seconds);
    }
    
    // Get temperature
    float temp = temperatureRead();
    
    // Get status
    String status;
    switch (mMonitor.NerdStatus) {
        case NM_waitingConfig:
            status = "Waiting Config";
            break;
        case NM_Connecting:
            status = "Connecting";
            break;
        case NM_hashing:
            status = "Mining";
            break;
        default:
            status = "Unknown";
    }
    
    // Build JSON response
    doc["hashRate"] = hashRateStr;
    doc["valids"] = valids;
    doc["shares"] = shares;
    doc["temperature"] = String(temp, 1);
    doc["uptime"] = uptimeStr;
    doc["bestDiff"] = String(best_diff, 2);
    doc["status"] = status;
    doc["poolUrl"] = Settings.PoolAddress;
    doc["poolPort"] = Settings.PoolPort;
    doc["btcAddress"] = Settings.BtcWallet;
    doc["totalKHashes"] = currentKHashes;
    doc["templates"] = templates;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["wifiSignal"] = WiFi.RSSI();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void setupWebServer() {
    // Serve the main HTML page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });
    
    // Serve the stats API endpoint
    server.on("/api/stats", HTTP_GET, handleStatsAPI);
    
    // Handle 404
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
    
    server.begin();
    Serial.println("Web server started on port 80");
    Serial.print("Access stats at: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
}

void handleWebServerClient() {
    // AsyncWebServer handles everything automatically
    // This function is here for compatibility
}

#endif // ENABLE_WEB_STATS
