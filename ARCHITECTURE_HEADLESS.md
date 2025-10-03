# Headless Mode Architecture Documentation

This document describes the technical architecture of the ESP32 WROOM headless mining implementation.

## System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    NerdMiner v2 - Headless Mode              │
└─────────────────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
    ┌───▼────┐       ┌─────▼──────┐      ┌────▼─────┐
    │ Mining │       │  Display   │      │   Web    │
    │ Engine │       │  Driver    │      │  Server  │
    │(Existing)      │ (Headless) │      │  (New)   │
    └───┬────┘       └─────┬──────┘      └────┬─────┘
        │                  │                   │
        └──────────────────┴───────────────────┘
                            │
                    ┌───────▼────────┐
                    │   Statistics   │
                    │   Variables    │
                    │  (Shared Data) │
                    └────────────────┘
```

## Component Architecture

### 1. Core Components

#### Mining Engine (Existing)
- Location: `src/mining.cpp`, `src/stratum.cpp`
- Handles actual mining operations
- Maintains statistics (shares, hashes, templates)
- Unchanged by headless implementation

#### Display Driver (New - Headless)
- Location: `src/drivers/displays/headlessDisplayDriver.cpp`
- Replaces traditional display output
- Implements `DisplayDriver` interface
- Responsibilities:
  - Initialize LED indicators
  - Log status to serial
  - Initialize web server on WiFi connect
  - Periodic status updates

#### Web Server (New)
- Location: `src/webServer.cpp`, `src/webServer.h`
- Provides HTTP interface for monitoring
- Built on ESPAsyncWebServer
- Non-blocking operation

### 2. Data Flow

```
Mining Loop (Core 0)                 Display Update (Core 1)
     │                                      │
     │ Updates Statistics                  │
     ├──────────────────────────────────►  │
     │ (shares, hashes, etc.)              │
     │                                      │
     │                                      ▼
     │                              ┌──────────────┐
     │                              │   Headless   │
     │                              │   Display    │
     │                              │   Driver     │
     │                              └──────┬───────┘
     │                                     │
     │                                     │ Triggers
     │                                     ▼
     │                              ┌──────────────┐
     │                              │  Web Server  │
     │                              │  Initialize  │
     │                              └──────┬───────┘
     │                                     │
     │                                     │ Serves
     │                                     ▼
     │                              ┌──────────────┐
     │                              │   Browser    │
     │                              │   Requests   │
     │                              └──────┬───────┘
     │                                     │
     │                                     │ Reads
     │                                     ▼
     └───────────────────────────► Statistics Vars
                                    (shared memory)
```

### 3. File Organization

```
NerdMiner_v2/
├── platformio.ini                    # Build configuration
├── HEADLESS_MODE.md                  # User documentation
├── TESTING_HEADLESS.md               # Testing guide
├── ARCHITECTURE_HEADLESS.md          # This file
│
├── src/
│   ├── webServer.h                   # Web server API
│   ├── webServer.cpp                 # Web server implementation
│   │
│   ├── drivers/
│   │   ├── devices/
│   │   │   ├── device.h             # Device selection (modified)
│   │   │   └── esp32WroomHeadless.h # Headless device config
│   │   │
│   │   └── displays/
│   │       ├── display.cpp          # Display interface (modified)
│   │       ├── displayDriver.h      # Driver declarations (modified)
│   │       └── headlessDisplayDriver.cpp  # Headless implementation
│   │
│   ├── mining.cpp                    # Mining engine (unchanged)
│   ├── monitor.cpp                   # Statistics (unchanged)
│   └── NerdMinerV2.ino.cpp          # Main entry point (unchanged)
```

## Class and Interface Design

### DisplayDriver Interface

```cpp
typedef struct {
  DriverInitFunction initDisplay;           // Setup hardware/pins
  AlternateFunction alternateScreenState;   // Toggle display on/off
  AlternateFunction alternateScreenRotation; // Rotate display
  ScreenFunction loadingScreen;             // Show loading
  ScreenFunction setupScreen;               // Show setup mode
  CyclicScreenFunction *cyclic_screens;     // Array of screen functions
  AnimateCurrentScreenFunction animateCurrentScreen;
  DoLedStuff doLedStuff;                   // LED control
  int num_cyclic_screens;                   // Screen count
  int current_cyclic_screen;                // Current screen index
  int screenWidth;                          // Display width
  int screenHeight;                         // Display height
} DisplayDriver;
```

### Headless Implementation

```cpp
// Function implementations
void headless_Init(void);                 // Initialize LED, serial
void headless_AlternateScreenState(void); // Toggle LED
void headless_AlternateRotation(void);    // No-op
void headless_NoScreen(unsigned long);    // Main update function
void headless_LoadingScreen(void);        // Serial "Loading..."
void headless_SetupScreen(void);          // Serial "Setup..."
void headless_DoLedStuff(unsigned long);  // LED status indicators
void headless_AnimateCurrentScreen(unsigned long); // No-op

// Screen array
CyclicScreenFunction headlessCyclicScreens[] = {
  headless_NoScreen
};

// Driver structure
DisplayDriver headlessDisplayDriver = {
  headless_Init,
  headless_AlternateScreenState,
  headless_AlternateRotation,
  headless_LoadingScreen,
  headless_SetupScreen,
  headlessCyclicScreens,
  headless_AnimateCurrentScreen,
  headless_DoLedStuff,
  1,  // num_cyclic_screens
  0,  // current_cyclic_screen
  0,  // screenWidth
  0,  // screenHeight
};
```

## Web Server Architecture

### HTTP Endpoints

```
/                     # Main dashboard (HTML)
/api/stats           # JSON statistics endpoint
*                    # 404 handler
```

### Request Flow

```
Browser Request
      │
      ▼
AsyncWebServer
      │
      ├──> "/" ──────────────► Serve HTML (PROGMEM)
      │
      ├──> "/api/stats" ─────► handleStatsAPI()
      │                             │
      │                             ├─► Read mining variables
      │                             ├─► Calculate hash rate
      │                             ├─► Format uptime
      │                             ├─► Get temperature
      │                             ├─► Build JSON response
      │                             └─► Send response
      │
      └──> Other ─────────────► 404 Not Found
```

### JSON Response Schema

```json
{
  "hashRate": "string (XX.XX KH/s)",
  "valids": number,
  "shares": number,
  "temperature": "string (XX.X)",
  "uptime": "string (XdXhXmXs)",
  "bestDiff": "string (X.XX)",
  "status": "string (Mining|Connecting|Waiting Config)",
  "poolUrl": "string",
  "poolPort": number,
  "btcAddress": "string",
  "totalKHashes": number,
  "templates": number,
  "freeHeap": number,
  "wifiSignal": number
}
```

## Memory Management

### Static Memory Allocation

```cpp
// Web server instance (heap)
AsyncWebServer server(80);

// HTML content (flash memory - PROGMEM)
const char index_html[] PROGMEM = R"rawliteral(
  ... ~10KB HTML content ...
)rawliteral";

// LED state (static)
bool ledOn = true;
unsigned long previousMillis = 0;
bool webServerInitialized = false;
```

### Dynamic Memory Usage

- **AsyncWebServer**: ~20-30KB (managed by library)
- **JSON Document**: ~1KB per request (stack allocated)
- **Request Buffers**: ~4KB per concurrent request

**Total Overhead**: ~30-50KB RAM

## Threading Model

### ESP32 Dual Core Usage

```
Core 0 (Protocol CPU)           Core 1 (Application CPU)
├── Mining Tasks                ├── WiFi Stack
│   ├── Miner 1                 ├── AsyncWebServer
│   └── Miner 2                 ├── Display Updates
│                               └── LED Control
└── Stratum Worker              
```

### Task Priorities

```
Priority 5: Monitor Task (Display updates)
Priority 4: Stratum Worker (Network I/O)
Priority 3: Web Server (HTTP handling - managed by AsyncTCP)
Priority 1: Mining Tasks (Hash computation)
```

## Initialization Sequence

```
1. setup() [NerdMinerV2.ino.cpp]
   │
   ├─► initDisplay()
   │   └─► headless_Init()
   │       ├─► Serial.println("Headless Mode")
   │       ├─► pinMode(LED_PIN, OUTPUT)
   │       └─► LED status indicators setup
   │
   ├─► drawLoadingScreen()
   │   └─► headless_LoadingScreen()
   │       └─► Serial.println("Loading...")
   │
   ├─► init_WifiManager()
   │   ├─► Connect to saved WiFi or
   │   └─► Start AP for configuration
   │
   ├─► Create Monitor Task (Priority 5)
   │   └─► Calls drawCurrentScreen() periodically
   │       └─► headless_NoScreen()
   │           ├─► Check WiFi connected
   │           └─► If connected: setupWebServer()
   │               ├─► Register HTTP routes
   │               ├─► Start server on port 80
   │               └─► Print IP address
   │
   ├─► Create Stratum Task (Priority 4)
   │   └─► Connect to mining pool
   │
   └─► Create Mining Tasks (Priority 1)
       ├─► Miner 1 on Core 1
       └─► Miner 2 on Core 1
```

## State Machine

### Miner States (NMState)

```
┌──────────────────┐
│ NM_waitingConfig │  LED: Solid ON
└────────┬─────────┘
         │ WiFi configured
         ▼
┌──────────────────┐
│  NM_Connecting   │  LED: Slow blink (0.5s)
└────────┬─────────┘
         │ Connected to pool
         ▼
┌──────────────────┐
│   NM_hashing     │  LED: Fast blink (0.1s)
└────────┬─────────┘
         │ Disconnect
         ▼
    (back to Connecting)
```

### Web Server States

```
┌─────────────────┐
│ Not Initialized │
└────────┬────────┘
         │ WiFi.status() == WL_CONNECTED
         ▼
┌─────────────────┐
│  Initializing   │
│  setupWebServer()
└────────┬────────┘
         │ server.begin()
         ▼
┌─────────────────┐
│    Running      │
│ Serving Requests
└─────────────────┘
```

## Conditional Compilation

### Build Flags

```
ESP32_WROOM_HEADLESS=1
    ↓
HEADLESS_DISPLAY defined in device header
    ↓
headlessDisplayDriver.cpp compiled
    ↓
display.cpp selects headless driver
```

### Preprocessor Flow

```cpp
// device.h
#ifdef ESP32_WROOM_HEADLESS
  #include "esp32WroomHeadless.h"
#endif

// esp32WroomHeadless.h
#define HEADLESS_DISPLAY

// display.cpp
#ifdef HEADLESS_DISPLAY
  DisplayDriver *currentDisplayDriver = &headlessDisplayDriver;
#endif

// headlessDisplayDriver.cpp
#ifdef HEADLESS_DISPLAY
  // Implementation
#endif
```

## Web Interface Architecture

### Frontend Structure

```
HTML Page
├── CSS (Inline)
│   ├── Glass-morphism design
│   ├── Gradient backgrounds
│   ├── Responsive grid layout
│   └── Animations (pulse, hover)
│
├── Stats Cards
│   ├── Hash Rate
│   ├── Valid Shares
│   ├── Total Shares
│   ├── Temperature
│   ├── Uptime
│   └── Best Difficulty
│
├── Info Section
│   ├── Status (with indicator)
│   ├── Pool information
│   ├── BTC address
│   ├── System stats
│   └── Network info
│
└── JavaScript
    ├── updateStats() function
    ├── fetch('/api/stats')
    ├── Update DOM elements
    └── setInterval(5000) for auto-refresh
```

### Frontend-Backend Communication

```
JavaScript (Browser)
       │
       │ fetch('/api/stats')
       ▼
ESPAsyncWebServer
       │
       │ handleStatsAPI()
       ▼
Read Mining Variables
(shares, hashes, etc.)
       │
       │ Build JSON
       ▼
Response to Browser
       │
       │ JSON data
       ▼
JavaScript Updates DOM
       │
       ▼
User sees updated stats
```

## Performance Characteristics

### CPU Usage

- Mining: ~95% (both cores when hashing)
- Web Server: <1% average, ~5% during requests
- Display Updates: <1%
- WiFi Stack: <2%

### Memory Usage

```
Flash (Code):
  Baseline:        ~1.2 MB
  + Headless:      ~1.3 MB  (+100 KB)
  + Web Server:    ~1.4 MB  (+100 KB)
  Total:           ~1.4 MB

RAM (Heap):
  Baseline:        ~200 KB free
  - Headless:      ~200 KB free (same)
  - Web Server:    ~150 KB free (-50 KB)
```

### Network Bandwidth

- Stratum: ~1-2 KB/s continuous
- Web Page: ~15 KB one-time per load
- API Request: ~0.5 KB per request
- Total: Minimal impact (<1% of typical WiFi)

## Security Considerations

### Current Implementation

- ✅ No authentication required (local network only)
- ✅ Read-only access (no control endpoints)
- ✅ No sensitive data exposure (BTC address is public)
- ✅ No CORS restrictions (same-origin only)

### Future Enhancements (Not Implemented)

- Basic authentication
- HTTPS/TLS support
- API rate limiting
- Configuration endpoints (with auth)
- WebSocket for real-time updates

## Extension Points

### Adding New Endpoints

```cpp
// In setupWebServer()
server.on("/api/newdata", HTTP_GET, [](AsyncWebServerRequest *request) {
  // Build response
  request->send(200, "application/json", jsonResponse);
});
```

### Custom Statistics

```cpp
// Add to handleStatsAPI()
doc["customStat"] = getCustomValue();
```

### Integration Hooks

The architecture supports easy integration:

1. **Home Assistant**: Use RESTful sensor
2. **Prometheus**: Scrape /api/stats endpoint
3. **Grafana**: Parse JSON for visualization
4. **Custom Apps**: Standard HTTP/JSON API

## Compatibility Matrix

| Feature | Headless | Standard Displays |
|---------|----------|-------------------|
| Mining | ✅ Full | ✅ Full |
| LED Status | ✅ Yes | ✅ Yes |
| Serial Output | ✅ Enhanced | ✅ Basic |
| Web Interface | ✅ Full | ❌ No |
| Visual Display | ❌ No | ✅ Yes |
| Touch Input | ❌ No | ⚠️ Some |
| Button Input | ✅ Yes | ✅ Yes |
| Memory Usage | ✅ Low | ⚠️ Higher |

## Design Decisions

### Why AsyncWebServer?

- **Non-blocking**: Doesn't interfere with mining
- **Efficient**: Lower memory/CPU overhead
- **Mature**: Well-tested ESP32 library
- **Active**: Good community support

### Why PROGMEM for HTML?

- **Memory**: Saves RAM by storing in flash
- **Performance**: Faster than SPIFFS reads
- **Simplicity**: No filesystem management
- **Size**: HTML is only ~10KB

### Why JSON API?

- **Standard**: Universal compatibility
- **Parseable**: Easy integration
- **Extensible**: Can add fields without breaking
- **Lightweight**: Minimal overhead

### Why Serial Logging?

- **Debugging**: Essential for headless debugging
- **Status**: Monitor without network
- **Compatibility**: Standard tool support
- **Fallback**: Always available

## Testing Strategy

### Unit Testing (Manual)
- LED indicators at each state
- Serial output formatting
- Web server initialization
- API response validation

### Integration Testing
- WiFi connection handling
- Web server + mining concurrency
- Memory leak detection
- Long-term stability

### Performance Testing
- Hash rate comparison (with/without web server)
- Memory usage monitoring
- Request handling under load
- Temperature under load

## Troubleshooting Guide

### Common Issues and Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| Web server not starting | WiFi not connected | Check WiFi credentials |
| Page not loading | Wrong IP address | Check serial output |
| Stats not updating | JavaScript error | Check browser console |
| High temperature | Poor ventilation | Add cooling |
| Low hash rate | CPU overload | Check background tasks |

## Future Enhancements

### Potential Additions

1. **WebSocket Support**: Real-time updates without polling
2. **Configuration UI**: Change settings via web
3. **Charts**: Historical hash rate graphs
4. **Themes**: Light/dark mode toggle
5. **Authentication**: Password protection
6. **mDNS**: Access via hostname
7. **OTA Updates**: Firmware updates via web
8. **Multi-Language**: Internationalization

### Community Contributions Welcome

- UI improvements
- Additional statistics
- Integration examples
- Performance optimizations
- Documentation improvements

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Maintainer**: NerdMiner Community
