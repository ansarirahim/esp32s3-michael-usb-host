# ESP32-S3 USB Host Automator - Quick Reference

## 🚀 Quick Start (One-Click Build & Flash)

**Just double-click this file:**
```
BUILD_AND_FLASH_NOW.bat
```

This will automatically:
1. Initialize ESP-IDF environment
2. Build the firmware
3. Flash to COM14 (Espressif - GPIO38)
4. Flash to COM11 (EWeAct - GPIO48)

---

## 📺 Monitor Serial Output

**Monitor COM14 (Espressif):**
```
Double-click: MONITOR_COM14.bat
```

**Monitor COM11 (EWeAct):**
```
Double-click: MONITOR_COM11.bat
```

**Exit Monitor:** Press `Ctrl + ]`

---

## 🛠️ Manual Build Commands

If you prefer manual control, use `build-idf.bat`:

### Build Only
```cmd
build-idf.bat build
```

### Flash to Specific Board
```cmd
build-idf.bat flash-com14    # Flash to COM14 (Espressif)
build-idf.bat flash-com11    # Flash to COM11 (EWeAct)
build-idf.bat flash-both     # Flash to both boards
```

### Monitor
```cmd
build-idf.bat monitor-com14  # Monitor COM14
build-idf.bat monitor-com11  # Monitor COM11
```

### Full Workflow
```cmd
build-idf.bat full           # Clean + Build + Flash COM14 + Monitor
build-idf.bat full-both      # Clean + Build + Flash BOTH + Monitor COM14
```

### Clean Build
```cmd
build-idf.bat clean
```

---

## 📋 Board Configuration

| Board | COM Port | GPIO Pin | MAC Address Pattern |
|-------|----------|----------|---------------------|
| Espressif | COM14 | GPIO38 | 34:85:18:xx:xx:xx |
| EWeAct | COM11 | GPIO48 | 64:e8:33:xx:xx:xx |

---

## ✅ What to Verify in Serial Output

After flashing, check the serial monitor for:

1. **MAC Address Detection**
   ```
   MAC Address: XX:XX:XX:XX:XX:XX
   ```

2. **Board Type Detection**
   ```
   Board detected: Espressif (GPIO38)
   or
   Board detected: EWeAct (GPIO48)
   ```

3. **GPIO Pin Configuration**
   ```
   LED GPIO: 38 (for Espressif)
   or
   LED GPIO: 48 (for EWeAct)
   ```

4. **No Errors**
   - Look for any error messages
   - Verify the board boots successfully

---

## 🔧 Troubleshooting

### Build Fails
- Make sure ESP-IDF is installed at: `C:\Users\Abdul\esp-idf-v5.5.1`
- If different location, edit the `IDF_PATH` in the batch files

### Flash Fails
- Check if boards are connected to correct COM ports
- Verify COM14 and COM11 in Device Manager
- Try pressing BOOT button while flashing

### Monitor Shows Garbage
- Check baud rate (should be 115200)
- Try resetting the board

---

## 📁 Key Files

- `BUILD_AND_FLASH_NOW.bat` - One-click build and flash
- `build-idf.bat` - Full-featured build script
- `MONITOR_COM14.bat` - Monitor Espressif board
- `MONITOR_COM11.bat` - Monitor EWeAct board
- `main/board_pins.c` - Board detection logic
- `main/main.c` - Main application code

---

## 🎯 Next Steps

1. ✅ Build and flash completed
2. 🔍 Monitor serial output to verify board detection
3. 🧪 Test LED control functionality
4. 📝 Verify GPIO auto-detection works correctly

---

**Note:** All scripts automatically initialize the ESP-IDF environment. No manual setup required!

