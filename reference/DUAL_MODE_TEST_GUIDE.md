# ESP32-S3 Dual Mode Testing Guide

## Current Status ✅
- **Board**: EWeAct ESP32-S3-DevKitC-1
- **Port**: COM11
- **RGB LED**: GPIO 48 (correctly configured)
- **Current Mode**: USB HOST MODE
- **Firmware**: Running successfully

## System Information
```
Board MAC: 48:ca:43:af:1e:40
LED GPIO: 48
ESP-IDF: v5.5.1
Project: esp32s3_michael_usb_host
```

## Two Operating Modes

### 1. 🔌 CDC HOST MODE (Current Mode)
**Purpose**: Automatically program LPC microcontrollers (SwissMicros calculators)

**What it does**:
- Acts as USB host
- Connects to USB-to-UART converters (CP210x, FTDI, CH34x)
- Automatically detects and programs LPC devices
- Reads firmware from `firmware.hex` file in internal storage
- Uses lpc21isp protocol at 115200 baud

**LED Indicators**:
- 🟣 **Magenta (solid)**: Waiting for USB device
- 🟡 **Yellow (blinking)**: Synchronizing with bootloader
- 🔵 **Blue (rapid blink)**: Programming in progress
- 🟢 **Green (solid)**: Programming successful
- 🔴 **Red (solid)**: Error occurred

**Testing CDC HOST Mode**:
1. Connect a USB-to-UART converter to the ESP32-S3 USB port
2. Watch the LED turn magenta (waiting)
3. If an LPC device is connected, it will attempt to program it

---

### 2. 💾 MSC DEVICE MODE
**Purpose**: Appear as USB mass storage device for file management

**What it does**:
- Acts as USB device (mass storage)
- Appears as removable drive on your computer
- Allows copying firmware files to the device
- Files stored in internal SPIFFS partition (256KB)

**Current Files in Storage**:
```
README.TXT    (276 bytes)
CONFIG.INI    (126 bytes)
DATA.BIN      (1024 bytes)
LOG.TXT       (436 bytes)
TEST.DAT      (4096 bytes)
fatlabel.txt  (11 bytes)
Total: 6 files, 5969 bytes
```

**Testing MSC DEVICE Mode**:
1. Switch to MSC mode (see below)
2. Connect ESP32-S3 to computer via USB
3. A new drive should appear (labeled "MICHAEL-USB")
4. You can read/write files to this drive

---

## 🔄 How to Switch Between Modes

### Long-Press BOOT Button Method
1. **Press and HOLD** the BOOT button (GPIO 0) for **2 seconds**
2. Watch the serial monitor for confirmation:
   ```
   I (xxx) app: BOOT0 button pressed for 2 seconds - toggling mode
   I (xxx) app: Current mode: CDC_HOST -> New mode: MSC_DEVICE
   I (xxx) app: Mode successfully saved to NVRAM
   I (xxx) app: Resetting MCU to apply new mode...
   ```
3. The device will automatically restart
4. After restart, it will be in the opposite mode

**Note**: The mode is saved to non-volatile memory (NVS), so it persists across power cycles.

---

## Testing Procedure

### Test 1: Verify Current Mode (CDC HOST)
✅ **Already confirmed** - Device is in CDC HOST mode
- Serial monitor shows: "Current Device Mode: CDC_HOST"
- LED should be visible on GPIO 48
- System is waiting for USB device connection

### Test 2: Switch to MSC DEVICE Mode
**Steps**:
1. **Press and HOLD** the BOOT button on your ESP32-S3 board for **2 seconds**
2. Watch the serial monitor for messages like:
   ```
   I (xxx) app: BOOT0 button pressed for 2 seconds - toggling mode
   I (xxx) app: Current mode: CDC_HOST (value: 0) -> New mode: MSC_DEVICE (value: 1)
   I (xxx) app: Mode successfully saved to NVRAM: MSC_DEVICE (value: 1)
   I (xxx) app: Resetting MCU to apply new mode...
   ```
3. Device will restart automatically
4. After restart, serial monitor should show:
   ```
   I (xxx) app: Current Device Mode: MSC_DEVICE
   I (xxx) app: Starting MSC_DEVICE mode...
   ```

### Test 3: Verify MSC DEVICE Mode
**Steps**:
1. After switching to MSC mode, connect ESP32-S3 to your computer via USB
2. A new removable drive should appear in Windows Explorer
3. Drive label should be "MICHAEL-USB"
4. You should see the 6 files listed above
5. Try creating a new text file on the drive
6. Try reading the README.TXT file

### Test 4: Switch Back to CDC HOST Mode
**Steps**:
1. While in MSC mode, **press and HOLD** the BOOT button for **2 seconds**
2. Device will restart
3. Verify it's back in CDC HOST mode from serial monitor

### Test 5: Mode Persistence Test
**Steps**:
1. Switch to MSC mode
2. Disconnect power from ESP32-S3
3. Reconnect power
4. Verify it's still in MSC mode (mode should persist)

---

## Troubleshooting

### RGB LED Not Working
- **Current config**: GPIO 48 ✅
- If LED doesn't light up, it might not be a WS2812B LED
- Check if your board has a different LED type
- The code will continue to work even if LED fails

### Mode Switch Not Working
- Make sure you're pressing the BOOT button (GPIO 0)
- Try the long-press method (hold for 2 seconds)
- Check serial monitor for button press detection messages

### MSC Drive Not Appearing
- Make sure you're in MSC DEVICE mode (check serial monitor)
- Try a different USB cable
- Try a different USB port on your computer
- Check Windows Device Manager for USB devices

### Serial Monitor Disconnects
- This is normal when switching modes
- Reconnect the serial monitor after mode switch
- Use: `python -m serial.tools.miniterm COM11 115200`

---

## Expected Serial Monitor Output

### CDC HOST Mode Startup:
```
I (xxx) mode_manager: Initializing USB Mode Manager (initial mode: HOST)
I (xxx) mode_manager: Initializing USB Host mode...
I (xxx) usb_host: ✓ USB Host initialized successfully
I (xxx) app: ✓ USB Mode Manager initialized successfully (mode: HOST)
```

### MSC DEVICE Mode Startup:
```
I (xxx) mode_manager: Initializing USB Mode Manager (initial mode: DEVICE)
I (xxx) mode_manager: Initializing USB Device mode...
I (xxx) usb_device: ✓ USB Device initialized successfully
I (xxx) app: ✓ USB Mode Manager initialized successfully (mode: DEVICE)
```

### Mode Switch Event:
```
I (xxx) app: BOOT0 button pressed for 2 seconds - toggling mode
I (xxx) app: Current mode: CDC_HOST (value: 0) -> New mode: MSC_DEVICE (value: 1)
I (xxx) app: Mode successfully saved to NVRAM: MSC_DEVICE (value: 1)
I (xxx) app: Resetting MCU to apply new mode...
```

---

## Next Steps

1. ✅ **Current**: Device is running in CDC HOST mode
2. ⏭️ **Next**: Press and HOLD BOOT button for 2 seconds to switch to MSC DEVICE mode
3. ⏭️ **Then**: Connect to PC and verify drive appears
4. ⏭️ **Finally**: Switch back to CDC HOST mode and test USB device detection

---

## Notes
- The device uses **NVS (Non-Volatile Storage)** to remember the mode
- Mode persists across power cycles and resets
- Both modes share the same internal SPIFFS storage (256KB)
- In CDC HOST mode, the device reads `firmware.hex` for programming
- In MSC DEVICE mode, you can upload new `firmware.hex` files

