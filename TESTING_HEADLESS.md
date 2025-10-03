# Testing Guide for ESP32 WROOM Headless Mode

This document provides testing procedures for the headless mode implementation.

## Prerequisites

- PlatformIO installed (CLI or IDE)
- ESP32 WROOM/DevKit board
- USB cable
- WiFi network for testing

## Build Testing

### 1. Compile the Headless Environment

```bash
cd NerdMiner_v2
pio run -e ESP32-WROOM-headless
```

**Expected Output:**
```
Processing ESP32-WROOM-headless (platform: espressif32@6.6.0; board: esp32dev; framework: arduino)
...
Building .pio/build/ESP32-WROOM-headless/firmware.bin
SUCCESS
```

### 2. Verify Build Artifacts

```bash
ls -lh .pio/build/ESP32-WROOM-headless/firmware.bin
```

**Expected Output:**
```
firmware.bin should be approximately 1-2 MB
```

## Flash and Functional Testing

### 1. Upload Firmware

```bash
pio run -e ESP32-WROOM-headless -t upload
```

### 2. Monitor Serial Output

```bash
pio device monitor -b 115200
```

**Expected Serial Output:**
```
==================================
NerdMiner v2 - Headless Mode
==================================
Initializing headless display driver...
LED Pin: 2
Display output redirected to Serial

[Display] Loading...
Please wait while NerdMiner initializes
```

### 3. WiFi Configuration

1. Look for WiFi AP in serial output:
   ```
   [Display] Setup Mode
   Waiting for configuration...
   Connect to the WiFi AP to configure settings
   ```

2. Connect to `NerdMinerAP` (password: `MineYourCoins`)

3. Configure via captive portal:
   - WiFi SSID: [Your WiFi]
   - WiFi Password: [Your Password]
   - Pool URL: `public-pool.io`
   - Pool Port: `21496`
   - BTC Address: [Your BTC Address]

4. Verify connection in serial:
   ```
   WiFi: Connected (-45 dBm)
   IP: 192.168.1.XXX
   ```

### 4. Web Server Testing

#### Check Web Server Start

Serial output should show:
```
==================================
Web Server Ready!
==================================
IP Address: 192.168.1.XXX
Access stats at: http://192.168.1.XXX/
==================================
```

#### Test Main Web Page

1. Open browser to `http://[IP_ADDRESS]/`
2. Verify page loads with NerdMiner stats
3. Check that statistics update every 5 seconds

**Expected Elements:**
- [ ] Page title: "NerdMiner v2 Stats"
- [ ] Hash Rate card showing KH/s
- [ ] Valid Shares counter
- [ ] Total Shares counter
- [ ] Temperature reading
- [ ] Uptime display
- [ ] Best Difficulty
- [ ] Mining status indicator
- [ ] Pool information
- [ ] BTC address display
- [ ] Auto-refresh working
- [ ] Manual refresh button functional

#### Test JSON API Endpoint

```bash
curl http://[IP_ADDRESS]/api/stats
```

**Expected Response:**
```json
{
  "hashRate": "XX.XX KH/s",
  "valids": 0,
  "shares": 0,
  "temperature": "XX.X",
  "uptime": "Xs",
  "bestDiff": "X.XX",
  "status": "Mining",
  "poolUrl": "public-pool.io",
  "poolPort": 21496,
  "btcAddress": "...",
  "totalKHashes": 0,
  "templates": 0,
  "freeHeap": XXXXXX,
  "wifiSignal": -XX
}
```

### 5. LED Indicator Testing

**Test Cases:**

| Status | LED Behavior | How to Verify |
|--------|--------------|---------------|
| Waiting Config | Solid ON | During initial setup |
| Connecting | Slow Blink (0.5s) | During WiFi/Pool connection |
| Mining | Fast Blink (0.1s) | When actively mining |

### 6. Mining Operation Testing

Monitor serial output for:
```
--- Mining Status ---
Status: Mining / Hashing
WiFi: Connected (-XX dBm)
IP: 192.168.1.XXX
Temperature: XX.X°C
Free Heap: XXXXXX bytes
--------------------
```

**Verify:**
- [ ] Status shows "Mining / Hashing"
- [ ] Hash rate increases over time
- [ ] Temperature readings are reasonable (50-70°C)
- [ ] Free heap doesn't decrease continuously (no memory leak)
- [ ] Templates counter increments
- [ ] Shares are found (may take time)

### 7. Web Interface Responsiveness

**Desktop Testing:**
1. Load page on desktop browser
2. Verify layout is clean and readable
3. Check stats cards are in grid layout
4. Verify refresh button works

**Mobile Testing:**
1. Load page on mobile device
2. Verify responsive design (single column on small screens)
3. Check text is readable without zoom
4. Verify buttons are touch-friendly

### 8. API Integration Testing

**Test with curl:**
```bash
# Continuous monitoring
watch -n 5 curl -s http://[IP_ADDRESS]/api/stats | jq .

# Save to file
curl http://[IP_ADDRESS]/api/stats > stats.json
```

**Test with Python:**
```python
import requests
import time

ip = "192.168.1.XXX"
url = f"http://{ip}/api/stats"

for i in range(10):
    response = requests.get(url)
    stats = response.json()
    print(f"Hash Rate: {stats['hashRate']}, Valids: {stats['valids']}")
    time.sleep(5)
```

## Regression Testing

### Verify Other Environments Still Build

Test a few standard environments to ensure no breaking changes:

```bash
# Test ESP32-devKitv1 (similar to headless but with NO_DISPLAY)
pio run -e ESP32-devKitv1

# Test a display-based environment
pio run -e TTGO-T-Display
```

**Expected:**
- [ ] All environments compile without errors
- [ ] No new warnings introduced
- [ ] Build sizes are reasonable

## Performance Testing

### Hash Rate Verification

1. Monitor web interface for 5 minutes
2. Record hash rates every 30 seconds
3. Calculate average

**Expected:** 40-50 KH/s (ESP32 WROOM)

### Memory Monitoring

```bash
# Monitor via serial for 30 minutes
# Check Free Heap doesn't decrease continuously
```

**Expected:**
- Free heap should stabilize (no memory leaks)
- Typically 100-200 KB free

### Temperature Monitoring

```bash
# Monitor temperature over 1 hour
```

**Expected:**
- Temperature: 50-70°C
- Should stabilize, not continuously increase

### WiFi Stability

```bash
# Monitor WiFi signal strength
# Check for disconnections
```

**Expected:**
- WiFi signal should remain stable
- Auto-reconnect if disconnected
- No excessive reconnection attempts

## Error Handling Testing

### Test WiFi Disconnect

1. Disable WiFi on router
2. Monitor serial output
3. Re-enable WiFi
4. Verify auto-reconnect

**Expected:**
- Miner detects disconnect
- Attempts reconnection
- Successfully reconnects
- Mining resumes

### Test Web Server Under Load

```bash
# Concurrent requests
for i in {1..10}; do
  curl http://[IP_ADDRESS]/api/stats &
done
wait
```

**Expected:**
- All requests complete successfully
- No crashes or hangs
- Response time < 1 second

### Test Invalid Requests

```bash
# Test 404 handling
curl http://[IP_ADDRESS]/invalid-path

# Test malformed requests
curl -X POST http://[IP_ADDRESS]/api/stats
```

**Expected:**
- 404 response for invalid paths
- Proper error handling, no crashes

## Documentation Verification

### Check Documentation

- [ ] HEADLESS_MODE.md exists and is complete
- [ ] README.md mentions headless mode
- [ ] Installation instructions are clear
- [ ] Troubleshooting section is helpful
- [ ] Code examples work as documented

### Verify Code Comments

```bash
# Check for proper documentation
grep -r "TODO\|FIXME\|XXX" src/webServer.cpp src/drivers/displays/headlessDisplayDriver.cpp
```

**Expected:** No critical TODOs or FIXMEs

## Security Testing

### Check for Sensitive Data Exposure

```bash
# Check API doesn't expose sensitive data
curl http://[IP_ADDRESS]/api/stats | grep -i "password\|secret\|key"
```

**Expected:** No sensitive data in API responses

## Sign-Off Checklist

- [ ] Build completes without errors
- [ ] Firmware uploads successfully
- [ ] Serial output shows correct initialization
- [ ] Web server starts and is accessible
- [ ] Web interface displays all statistics
- [ ] JSON API returns valid data
- [ ] LED indicators work correctly
- [ ] Mining operates normally
- [ ] Mobile interface is responsive
- [ ] No memory leaks detected
- [ ] Temperature stays within limits
- [ ] WiFi auto-reconnect works
- [ ] Other environments still build
- [ ] Documentation is complete
- [ ] No security issues found

## Known Issues

Document any known issues or limitations:

1. **Network Download**: PlatformIO may require network access to download dependencies on first build
2. **Build Time**: First build may take 5-10 minutes to download and compile dependencies
3. **Memory**: Web server requires ~30-50KB of RAM, ensure sufficient free heap

## Support

If you encounter issues during testing:

1. Check serial output for error messages
2. Verify WiFi configuration is correct
3. Check network connectivity
4. Review HEADLESS_MODE.md troubleshooting section
5. Open GitHub issue with:
   - Board type
   - Serial output logs
   - Network configuration
   - Build errors (if any)

## Contributing Test Results

When reporting test results, please include:

- Board model and revision
- PlatformIO version
- Test date
- All checklist items status
- Any issues encountered
- Performance metrics (hash rate, memory usage)

---

**Last Updated:** 2024
