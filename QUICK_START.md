# ⚡ QUICK START - Build & Flash Cheat Sheet

**Project:** ESP32-S3 USB Host Automator
**Date:** November 2025

---

## 🎯 Your Boards

```
COM14 (GPIO38) ← Espressif DevKitC-1-N8R8
COM11 (GPIO48) ← EWeAct ESP32-S3-DevKitC-1
```

**NO GPIO CHANGES NEEDED!** Firmware auto-detects which board! ✅

---

## 🚀 FASTEST WAY: Use Build Scripts

### **Option 1: Using build.bat (Windows CMD)**

```bash
# Open VS Code Terminal (Ctrl + `)
# Navigate to project folder

# Clean build
build.bat clean

# Build firmware
build.bat build

# Flash to COM14
build.bat flash-com14

# Flash to COM11
build.bat flash-com11

# Flash to BOTH
build.bat flash-both

# Monitor COM14
build.bat monitor-com14

# Monitor COM11
build.bat monitor-com11

# Full workflow (Clean + Build + Flash COM14 + Monitor)
build.bat full

# Full workflow (Clean + Build + Flash BOTH + Monitor COM14)
build.bat full-both
```

---

### **Option 2: Using build.ps1 (PowerShell)**

```bash
# Open VS Code Terminal (Ctrl + `)
# Navigate to project folder

# Clean build
.\build.ps1 clean

# Build firmware
.\build.ps1 build

# Flash to COM14
.\build.ps1 flash-com14

# Flash to COM11
.\build.ps1 flash-com11

# Flash to BOTH
.\build.ps1 flash-both

# Monitor COM14
.\build.ps1 monitor-com14

# Monitor COM11
.\build.ps1 monitor-com11

# Full workflow
.\build.ps1 full

# Full workflow (both boards)
.\build.ps1 full-both
```

---

## 📝 Direct idf.py Commands (If not using scripts)

```bash
# Clean
idf.py fullclean

# Build
idf.py build

# Flash COM14
idf.py -p COM14 flash

# Flash COM11
idf.py -p COM11 flash

# Flash BOTH
idf.py -p COM14 flash && idf.py -p COM11 flash

# Monitor COM14
idf.py -p COM14 monitor

# Monitor COM11
idf.py -p COM11 monitor
```

---

## 🎯 RECOMMENDED WORKFLOW

### **First Time Setup:**

```bash
# 1. Open VS Code Terminal (Ctrl + `)
# 2. Navigate to project
cd c:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host

# 3. Clean build
build.bat clean

# 4. Build
build.bat build

# 5. Flash to COM14
build.bat flash-com14

# 6. Monitor COM14 (see serial output)
build.bat monitor-com14

# 7. Exit monitor (Ctrl + ])

# 8. Flash to COM11
build.bat flash-com11

# 9. Monitor COM11 (see serial output)
build.bat monitor-com11

# 10. Exit monitor (Ctrl + ])
```

---

## 📊 Expected Serial Output

### **COM14 (Espressif - GPIO38):**
```
I (board_pins): Chip MAC address: 98:a3:16:f0:6c:64
I (board_pins): ✓ MAC matched: Espressif DevKitC-1-N8R8 → GPIO 38
I (board_pins): Board: Espressif DevKitC-1-N8R8
I (board_pins): LED GPIO: 38
```

### **COM11 (EWeAct - GPIO48):**
```
I (board_pins): Chip MAC address: 48:ca:43:af:1e:40
I (board_pins): ✓ MAC matched: EWeAct ESP32-S3-DevKitC-1 → GPIO 48
I (board_pins): Board: EWeAct ESP32-S3-DevKitC-1
I (board_pins): LED GPIO: 48
```

---

## ⚠️ Troubleshooting

| Problem | Solution |
|---------|----------|
| **Port not found** | Check USB cable, verify COM port in Device Manager |
| **Failed to connect** | Press BOOT button on board, try again |
| **Timeout error** | Disconnect USB, reconnect, try again |
| **Permission denied** | Run VS Code as Administrator |

---

## 🎯 QUICK REFERENCE

| What | Command |
|------|---------|
| **Clean** | `build.bat clean` |
| **Build** | `build.bat build` |
| **Flash COM14** | `build.bat flash-com14` |
| **Flash COM11** | `build.bat flash-com11` |
| **Flash BOTH** | `build.bat flash-both` |
| **Monitor COM14** | `build.bat monitor-com14` |
| **Monitor COM11** | `build.bat monitor-com11` |
| **Full (COM14)** | `build.bat full` |
| **Full (BOTH)** | `build.bat full-both` |

---

## 🚀 READY TO BUILD?

**Next Steps:**

1. Open VS Code Terminal (Ctrl + `)
2. Run: `build.bat clean && build.bat build`
3. Run: `build.bat flash-both`
4. Run: `build.bat monitor-com14`
5. Observe serial output
6. Switch to COM11: `build.bat monitor-com11`
7. Observe serial output

**That's it! You're ready to test Phase 1: LED Control! 🎉**

---

## 📚 For More Details

See: `BUILD_AND_FLASH_GUIDE.md`

---

**Let's build! 🚀**

