# ESP32 WROOM Headless Mining Configuration

This guide explains how to use NerdMiner v2 in headless mode (without display) on ESP32 WROOM/DevKit boards with web-based monitoring.

## Features

- 🚫 **No Display Required** - Run NerdMiner without any screen hardware
- 🌐 **Web-Based Stats** - Monitor mining statistics through a responsive web interface
- 📊 **Real-Time Updates** - Auto-refreshing stats every 5 seconds
- 📱 **Mobile-Friendly** - Responsive design works on any device
- 🔧 **Serial Monitoring** - Debug and status updates via serial output
- 🌡️ **Temperature Monitoring** - ESP32 internal temperature sensor
- 💾 **Low Memory Footprint** - Optimized for ESP32 WROOM boards

## Hardware Requirements

- ESP32 WROOM/DevKit board (any ESP32 with WiFi)
- USB cable for programming and power
- No display hardware required!

## Installation

### Using PlatformIO (Recommended)

1. Clone the repository:
   ```bash
   git clone https://github.com/vandsh/NerdMiner_v2.git
   cd NerdMiner_v2
   ```

2. Open the project in VS Code with PlatformIO extension installed

3. Select the `ESP32-WROOM-headless` environment from the PlatformIO toolbar

4. Build and upload:
   ```bash
   pio run -e ESP32-WROOM-headless -t upload
   ```

### Configuration

#### WiFi and Mining Pool Setup

On first boot, the NerdMiner will create a WiFi access point:

1. Connect to WiFi network `NerdMinerAP` (password: `MineYourCoins`)
2. The configuration portal should open automatically
3. If not, navigate to `http://192.168.4.1`
4. Configure:
   - **WiFi SSID** - Your WiFi network name
   - **WiFi Password** - Your WiFi password
   - **Pool URL** - Mining pool address (default: `public-pool.io`)
   - **Pool Port** - Mining pool port (default: `21496`)
   - **BTC Address** - Your Bitcoin wallet address
   - **Timezone** - Your timezone offset (e.g., 2 for GMT+2)

5. Save and the miner will connect to your network

#### Finding Your Miner's IP Address

After connecting to WiFi, check the serial monitor at 115200 baud to see:
```
==================================
Web Server Ready!
==================================
IP Address: 192.168.1.100
Access stats at: http://192.168.1.100/
==================================
```

## Web Interface

### Accessing the Stats Page

Open your browser and navigate to your miner's IP address:
```
http://YOUR_MINER_IP/
```

### Main Statistics Dashboard

The web interface displays:

#### Real-Time Stats Cards
- **Hash Rate** - Current mining speed in KH/s
- **Valid Shares** - Number of valid shares submitted
- **Total Shares** - Total shares found (valid + invalid)
- **Temperature** - ESP32 internal temperature in °C
- **Uptime** - Time since miner started
- **Best Difficulty** - Best share difficulty found

#### Mining Information Section
- **Status** - Current mining state (Mining/Connecting/Waiting)
- **Pool URL** - Connected mining pool address
- **Pool Port** - Mining pool port
- **BTC Address** - Your configured Bitcoin address
- **Total KHashes** - Cumulative hashes computed
- **Templates** - Number of mining templates received
- **Free Heap** - Available RAM in bytes
- **WiFi Signal** - Signal strength in dBm

### API Endpoint

For programmatic access, use the JSON API:

```
GET http://YOUR_MINER_IP/api/stats
```

**Response Example:**
```json
{
  "hashRate": "45.23 KH/s",
  "valids": 1234,
  "shares": 5678,
  "temperature": "56.7",
  "uptime": "2d 15h 32m 45s",
  "bestDiff": "12.45",
  "status": "Mining",
  "poolUrl": "public-pool.io",
  "poolPort": 21496,
  "btcAddress": "yourBtcAddress...",
  "totalKHashes": 123456,
  "templates": 42,
  "freeHeap": 156789,
  "wifiSignal": -45
}
```

## Serial Output

### Monitoring via Serial

Connect to the serial port at **115200 baud** to see:

- Initialization messages
- WiFi connection status
- IP address assignment
- Periodic mining status updates (every 30 seconds)
- Temperature readings
- Memory usage

**Example Serial Output:**
```
==================================
NerdMiner v2 - Headless Mode
==================================
Initializing headless display driver...
LED Pin: 2
Display output redirected to Serial

[Display] Loading...
Please wait while NerdMiner initializes

WiFi: Connected (-45 dBm)
IP: 192.168.1.100

--- Mining Status ---
Status: Mining / Hashing
WiFi: Connected (-45 dBm)
IP: 192.168.1.100
Temperature: 56.7°C
Free Heap: 156789 bytes
--------------------
```

## LED Indicators

The onboard LED (GPIO 2 by default) indicates status:

- **Solid ON** - Waiting for configuration
- **Slow Blink (0.5s)** - Connecting to WiFi/Pool
- **Fast Blink (0.1s)** - Mining/Hashing

## Troubleshooting

### Cannot Access Web Interface

1. Check serial output for IP address
2. Ensure your device is on the same network
3. Try pinging the miner: `ping YOUR_MINER_IP`
4. Check your router's DHCP client list

### WiFi Connection Issues

1. Verify WiFi credentials in configuration portal
2. Check WiFi signal strength (should be > -70 dBm)
3. Ensure 2.4GHz WiFi is enabled (ESP32 doesn't support 5GHz)
4. Try moving the miner closer to the router

### Mining Not Starting

1. Verify pool address and port are correct
2. Check that BTC address is valid
3. Ensure internet connection is working
4. Monitor serial output for error messages

### Web Page Not Loading

1. Clear browser cache
2. Try accessing `http://IP_ADDRESS/` with trailing slash
3. Verify web server started (check serial output)
4. Try accessing from different device

## Advanced Configuration

### Customizing Build Flags

Edit `platformio.ini` to customize:

```ini
build_flags = 
    -D ESP32_WROOM_HEADLESS=1
    -D DISABLE_DISPLAY=1
    -D ENABLE_WEB_STATS=1
    -D LED_PIN=2              ; Change LED pin
    -D PIN_BUTTON_1=0         ; Configure button pin
    ;-D DEBUG_MINING=1        ; Enable debug output
```

### Changing LED Pin

By default, GPIO 2 is used for the LED. To change:

1. Edit `src/drivers/devices/esp32WroomHeadless.h`
2. Modify `#define LED_PIN 2` to your desired pin
3. Rebuild and upload

### Performance Optimization

For optimal mining performance:
- Use stable power supply (minimum 500mA)
- Ensure good WiFi signal strength
- Monitor temperature (< 80°C recommended)
- Avoid excessive serial output in production

## Statistics and Monitoring

### Hash Rate Calculation

Hash rate is calculated based on:
- Total hashes computed
- Elapsed time since last calculation
- Displayed in KH/s (kilohashes per second)

### Expected Performance

ESP32 WROOM typical performance:
- **Hash Rate**: 40-50 KH/s
- **Power Consumption**: ~150-200mA @ 5V
- **Temperature**: 50-70°C under load

### Share Validation

- **Valid Shares**: Shares meeting pool difficulty
- **Total Shares**: All shares found (including below difficulty)
- **Best Difficulty**: Highest difficulty share ever found

## Integration Examples

### Home Assistant

Monitor your NerdMiner with Home Assistant REST sensor:

```yaml
sensor:
  - platform: rest
    name: "NerdMiner Stats"
    resource: "http://YOUR_MINER_IP/api/stats"
    scan_interval: 30
    json_attributes:
      - hashRate
      - valids
      - shares
      - temperature
      - uptime
      - status
    value_template: "{{ value_json.hashRate }}"
```

### Custom Dashboard

Use the JSON API to build custom monitoring dashboards:

```javascript
// Fetch stats every 5 seconds
setInterval(async () => {
  const response = await fetch('http://YOUR_MINER_IP/api/stats');
  const stats = await response.json();
  console.log('Hash Rate:', stats.hashRate);
  console.log('Valid Shares:', stats.valids);
}, 5000);
```

## Contributing

Found a bug or want to improve the headless mode? Contributions are welcome!

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is open source and available under the same license as NerdMiner v2.

## Support

- **GitHub Issues**: Report bugs or request features
- **Discord**: Join the NerdMiner community
- **Documentation**: Check the main README.md for general information

## Acknowledgments

- Original NerdMiner project by the NerdMiner community
- ESP32 Arduino Core developers
- ESPAsyncWebServer library contributors

---

**Happy Mining! ⛏️**
