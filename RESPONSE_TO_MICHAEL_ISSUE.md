# Response to Michael - USB Host Mode Issue

---

## Issue Summary

**Problem 1:** When connecting ESP32-S3 to computer, it shows as CDC device (serial port) instead of MSC device  
**Problem 2:** Files get copied but eject does not work  
**Problem 3:** Cannot switch to MSC device mode yet

---

## Root Cause Analysis

### Issue 1: CDC Device Showing

The ESP32-S3 has **USB-OTG** which can operate in two modes:
- **USB Device Mode** - ESP32 acts as a USB device (CDC, MSC, etc.) when connected to a computer
- **USB Host Mode** - ESP32 acts as a USB host to control USB flash drives

**Current Situation:**
- The firmware is configured for **USB Host Mode** (to control USB flash drives)
- When you connect the ESP32-S3 to a computer via USB, the computer sees it as a CDC device because:
  - The USB-Serial-JTAG peripheral is active (for debugging/logging)
  - This is **separate** from the USB-OTG peripheral used for USB Host

**This is NORMAL and EXPECTED behavior!**

### Issue 2: Eject Not Working

The "eject" function in the firmware is for **ejecting USB flash drives** that are connected TO the ESP32-S3, not for ejecting the ESP32-S3 itself from a computer.

**Clarification:**
- The ESP32-S3 is the **USB Host** (like a computer)
- The USB flash drive is the **USB Device** (like a thumb drive)
- The firmware ejects the USB flash drive from the ESP32-S3
- The firmware does NOT eject the ESP32-S3 from your computer

---

## Hardware Configuration

### Your Board vs Development Board

**Your Board (Michael's):**
- Unknown model (MAC: 7c:2c:67:8c:92:0c)
- LED GPIO: 48 ✓ (working after change)
- USB-OTG pins: GPIO 19 (D-), GPIO 20 (D+)

**Development Board (Abdul's):**
- EWeAct ESP32-S3-DevKitC-1 (MAC: 48:ca:43:af:1e:40)
- LED GPIO: 48
- USB-OTG pins: GPIO 19 (D-), GPIO 20 (D+)

### USB Connections on ESP32-S3

The ESP32-S3 has **TWO separate USB interfaces**:

1. **USB-Serial-JTAG** (Built-in)
   - Used for: Programming, debugging, serial monitor
   - Appears as: CDC device (COM port) on computer
   - Pins: GPIO 19, GPIO 20 (shared with USB-OTG)
   - **This is what you see when connecting to computer**

2. **USB-OTG** (For USB Host Mode)
   - Used for: Controlling USB flash drives
   - Appears as: USB Host controller
   - Pins: GPIO 19, GPIO 20 (shared with USB-Serial-JTAG)
   - **This is what the firmware uses**

**Important:** You **cannot use both at the same time** on the same pins!

---

## Solution and Clarification

### What the Firmware Does (USB Host Mode)

```
ESP32-S3 (USB Host)  ←→  USB Flash Drive (USB Device)
     ↑
     └─ Controls the flash drive
     └─ Formats, copies files, ejects
```

**Workflow:**
1. Insert USB flash drive into ESP32-S3 USB-OTG port
2. ESP32-S3 detects the drive
3. ESP32-S3 formats the drive (if needed)
4. ESP32-S3 copies files to the drive
5. ESP32-S3 ejects the drive
6. Remove USB flash drive
7. Insert next USB flash drive → repeat

### What You're Seeing (USB Device Mode)

```
Computer (USB Host)  ←→  ESP32-S3 (USB Device - CDC)
     ↑
     └─ Sees ESP32-S3 as serial port
     └─ Used for serial monitor/debugging
```

**This is for debugging only!**

---

## Hardware Setup Required

### Correct Connection for USB Host Mode

You need **TWO USB connections**:

**Connection 1: Power + Serial Monitor (USB-Serial-JTAG)**
- Connect ESP32-S3 **UART/Debug USB port** to computer
- This provides:
  - Power to ESP32-S3
  - Serial monitor output (PuTTY/Terminal)
  - Programming interface
- **This is the CDC device you see**

**Connection 2: USB Flash Drive (USB-OTG)**
- Connect USB flash drive to ESP32-S3 **USB-OTG port**
- This is where the firmware operates
- **This is the MSC device the firmware controls**

### Physical Setup

```
Computer ─[USB Cable]─→ ESP32-S3 Debug Port (Power + Serial)
                              ↓
                        ESP32-S3 Board
                              ↓
                        USB-OTG Port ←─[USB Cable]─ USB Flash Drive
```

**Note:** Most ESP32-S3 boards have:
- **One USB-C port** for programming/debug (USB-Serial-JTAG)
- **One USB-A port or header** for USB-OTG (USB Host)

OR

- **One USB-C port** that can be switched between modes using a jumper/switch

---

## Verification Steps

### Step 1: Check Your Board

1. Identify which USB port is USB-OTG on your board
2. Check if there's a jumper or switch to select USB mode
3. Consult your board's schematic or documentation

### Step 2: Test USB Host Mode

1. **Connect ESP32-S3 to computer** (for power and serial monitor)
2. **Open PuTTY** on the COM port (115200 baud)
3. **Press RESET** on ESP32-S3
4. **Wait for "Waiting for USB device..." message**
5. **Connect USB flash drive to USB-OTG port** (NOT to computer!)
6. **Observe serial output** - should show:
   - USB device detected
   - Formatting (if needed)
   - Copying files
   - Ejecting
   - "Waiting for next USB drive..."

### Step 3: Verify Files on USB Drive

1. After "Ejecting" message appears
2. **Remove USB flash drive from ESP32-S3**
3. **Connect USB flash drive to computer**
4. **Check files:**
   - README.TXT
   - CONFIG.INI
   - DATA.BIN
   - LOG.TXT
   - TEST.DAT
5. **Check volume label:** Should be "MICHAEL-USB"

---

## Common Misunderstandings

### ❌ Incorrect: "ESP32-S3 should appear as MSC device on computer"

**No!** The ESP32-S3 is the **USB Host** (like a computer). It controls USB flash drives. It does not appear as a mass storage device itself.

### ✅ Correct: "USB flash drive appears as MSC device to ESP32-S3"

**Yes!** The USB flash drive is the **USB Device**. The ESP32-S3 detects it, formats it, copies files to it, and ejects it.

---

## About "Eject Not Working"

### What "Eject" Means in the Firmware

The firmware's eject function performs:
1. Filesystem sync (flush write cache)
2. VFS unmount (close filesystem)
3. MSC driver uninstall (release USB driver)
4. USB device close (release USB handle)

**This ejects the USB flash drive from the ESP32-S3**, not the ESP32-S3 from your computer!

### How to Verify Eject is Working

Check the serial output for:
```
I (xxxxx) usb_host: =================================================
I (xxxxx) usb_host: Safe Eject: Starting...
I (xxxxx) usb_host: =================================================
I (xxxxx) usb_host: Step 1: Syncing filesystem...
I (xxxxx) usb_host: ✓ Filesystem sync delay completed
I (xxxxx) usb_host: Step 2: Unmounting VFS...
I (xxxxx) usb_host: ✓ VFS unmounted
I (xxxxx) usb_host: Step 3: Uninstalling MSC device...
I (xxxxx) usb_host: ✓ MSC device uninstalled
I (xxxxx) usb_host: Step 4: Closing USB device...
I (xxxxx) usb_host: ✓ USB device closed
I (xxxxx) usb_host: =================================================
I (xxxxx) usb_host: ✓ Safe Eject: COMPLETE
I (xxxxx) usb_host: =================================================
I (xxxxx) usb_host: USB drive can now be safely removed
```

If you see this output, **eject is working correctly!**

---

## Switching to MSC Device Mode (Optional)

### Question: "Can I switch to MSC device mode?"

**Answer:** Not with the current firmware. The firmware is designed for **USB Host Mode** only.

### If You Want ESP32-S3 to Act as MSC Device

This would require:
1. Different firmware (USB Device mode instead of USB Host mode)
2. Different use case (ESP32-S3 appears as USB drive to computer)
3. Different hardware configuration (USB-OTG in device mode)

**This is NOT what Michael requested!**

Michael's requirements were:
- ESP32-S3 controls USB flash drives (USB Host Mode) ✓
- Format drives with custom label ✓
- Copy files to drives ✓
- Eject drives safely ✓
- Process multiple drives automatically ✓

**All requirements are for USB Host Mode, which is implemented!**

---

## Recommended Response to Michael

```
Hi Michael,

Thank you for testing the firmware!

Regarding the CDC device you're seeing when connecting to your computer:

This is normal and expected! The ESP32-S3 has two USB interfaces:

1. USB-Serial-JTAG (for debugging) - This is the CDC device you see
2. USB-OTG (for USB Host) - This is what the firmware uses

The firmware is designed for USB Host Mode, where the ESP32-S3 controls USB flash drives (like a computer), not to act as a USB device itself.

Hardware Setup:
- Connect ESP32-S3 to computer via debug USB port (for power and serial monitor)
- Connect USB flash drive to ESP32-S3 USB-OTG port (separate port or header)
- The firmware will automatically process the USB flash drive

Regarding "eject not working":
The eject function works on the USB flash drive connected TO the ESP32-S3, not on the ESP32-S3 itself. If you see the eject messages in the serial output, it's working correctly!

Regarding "files get copied":
Great! This confirms the USB Host mode is working. The workflow is:
1. Insert USB drive into ESP32-S3
2. ESP32 formats and copies files
3. ESP32 ejects the drive
4. Remove USB drive from ESP32
5. Insert next USB drive → repeat

Could you please share the complete serial output (PuTTY log) so I can verify everything is working as expected?

Best regards,
Abdul
```

---

## Technical Notes

### USB-OTG Hardware Requirements

For USB Host mode to work, you need:
1. **USB-OTG resistor** (19.5kΩ to 21kΩ) on ID pin to GND
2. **5V power supply** capable of providing current for USB devices
3. **USB-OTG connector** (USB-A female or USB-C with OTG support)

**Question to Michael:** "Did you short the OTG resistor?"

This is critical! Without the OTG resistor, the ESP32-S3 won't enter USB Host mode.

### Checking OTG Resistor

On most ESP32-S3 boards:
- There's a solder jumper or 0Ω resistor labeled "OTG" or "HOST"
- You need to short/solder this jumper for USB Host mode
- Check your board's schematic or documentation

---

## Summary

| Item | Status | Notes |
|------|--------|-------|
| LED working | ✅ Fixed | GPIO 48 correct |
| CDC device showing | ✅ Normal | USB-Serial-JTAG for debugging |
| Files get copied | ✅ Working | USB Host mode functional |
| Eject not working | ❓ Unclear | Need serial output to verify |
| MSC device mode | ❌ Not applicable | Firmware is USB Host, not USB Device |

**Next Steps:**
1. Verify USB-OTG hardware setup (resistor, connector)
2. Share complete serial output for analysis
3. Confirm USB flash drive is connected to USB-OTG port (not computer)

---

*Document prepared for Michael Steinmann*  
*Date: November 8, 2025*  
*Issue: USB Host Mode Configuration*

