# ESP32-S3 USB Host Automator - Fresh Start

**Project:** USB Host Mode Automation for Michael Steinmann
**Status:** Fresh Start - Clean Implementation
**Date:** November 2025

---

## 🎯 Michael's Requirements

### **Step 1: Prepare USB Drive**
- **Option A:** Delete all files + Change label
- **Option B (Preferred):** Delete partition + Create new partition + Format with label

### **Step 2: Copy Files**
- Copy all files from `/storage/fatlabel.txt` directory to USB drive
- Sync filesystem
- Eject USB device

### **Step 3: Wait and Repeat**
- Wait for USB device removal
- Wait for new USB device insertion
- Repeat Step 1

### **Special Requirements:**
- Label name read from `fatlabel.txt` (not copied to USB)
- RGB LED visualizes current step
- Automatic workflow (no manual intervention)

---

## 📋 Project Structure

```
esp32s3-michael-usb-host/
├── README.md (this file)
├── .gitignore
├── CMakeLists.txt
├── sdkconfig.defaults
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   ├── board_pins.h
│   ├── led_control.h
│   ├── led_control.c
│   ├── filesystem.h
│   ├── filesystem.c
│   ├── usb_host.h
│   ├── usb_host.c
│   ├── usb_device.h
│   ├── usb_device.c
│   ├── usb_host_automator.h
│   └── usb_host_automator.c
├── docs/
│   ├── ARCHITECTURE.md
│   ├── IMPLEMENTATION_PLAN.md
│   ├── LED_STATES.md
│   └── TESTING_GUIDE.md
└── build/ (generated)
```

---

## 🚀 Implementation Phases

### **Phase 1: LED Control**
- Initialize WS2812B RGB LED on GPIO 38/48
- Implement RMT peripheral driver
- Define LED states for each step

### **Phase 2: USB Host Mode**
- Initialize USB Host stack
- Detect USB pen drive insertion
- Mount USB drive

### **Phase 3: USB Device Mode**
- Initialize USB Device stack (MSC)
- Expose internal FATFS as USB storage
- Handle device mode operations

### **Phase 4: USB Host Automator**
- Implement FSM for Michael's workflow
- Step 1: Prepare USB drive (delete partition + format)
- Step 2: Copy files + sync + eject
- Step 3: Wait and repeat

### **Phase 5: Integration & Testing**
- Test complete workflow
- Verify LED indicators
- Document and deliver

---

## 📊 LED States

| State | Color | Pattern | Meaning |
|-------|-------|---------|---------|
| IDLE | Green | Slow blink | Waiting for USB |
| PREPARE | Cyan | Fast blink | Preparing USB drive |
| COPY | Yellow | Blink | Copying files |
| SYNC | Magenta | Blink | Syncing filesystem |
| SUCCESS | Green | Solid 2s | Operation complete |
| ERROR | Red | Fast blink | Error occurred |

---

## 🔧 Build & Flash

```bash
# Build
idf.py build

# Flash
idf.py flash

# Monitor
idf.py monitor
```

---

## 📝 Lessons Learned from Previous Work

1. **OTG Resistor Pads:** May need to be shorted for USB Host Mode
2. **5V VBUS Power:** Required for USB device detection
3. **Board-to-Board Connection:** Can use one board as power source
4. **RMT Peripheral:** Better than GPIO for WS2812B LED control
5. **FatFS:** Reliable for internal storage
6. **TinyUSB:** Supports both Host and Device modes

---

## ✅ Acceptance Criteria

- [ ] LED initializes on correct GPIO (auto-detected)
- [ ] USB pen drive detected when inserted
- [ ] USB drive formatted with correct label
- [ ] Files copied successfully
- [ ] Filesystem synced
- [ ] USB device ejected safely
- [ ] Workflow repeats automatically
- [ ] LED shows correct state for each step
- [ ] Error handling works
- [ ] Documentation complete

---

## 🎯 Next Steps

1. Set up fresh ESP-IDF project
2. Implement Phase 1: LED Control
3. Implement Phase 2: USB Host Mode
4. Implement Phase 3: USB Device Mode
5. Implement Phase 4: USB Host Automator
6. Test and verify
7. Deliver to Michael

---

**Ready to start? Let's build this clean and professional!**

