# 🔨 Build & Flash Guide - ESP32-S3 USB Host Automator

**Project:** ESP32-S3 USB Host Automator
**Date:** November 2025
**Purpose:** Easy build, clean, and flash to both boards from VS Code terminal

---

## 🎯 Your Boards

| Board | COM Port | GPIO | MAC Address | Board Name |
|-------|----------|------|-------------|-----------|
| **Board A** | COM14 | 38 | 98:a3:16:f0:6c:64 | Espressif DevKitC-1-N8R8 |
| **Board B** | COM11 | 48 | 48:ca:43:af:1e:40 | EWeAct ESP32-S3-DevKitC-1 |

---

## ✅ How GPIO Auto-Detection Works

**NO NEED TO CHANGE GPIO!** The firmware automatically detects which board it's running on:

```c
// board_pins.c automatically detects:
// - Reads MAC address from chip
// - Matches against board table
// - Sets GPIO38 for COM14 (Espressif)
// - Sets GPIO48 for COM11 (EWeAct)
```

**Result:** Same firmware works on BOTH boards! ✅

---

## 🔨 Build Commands (VS Code Terminal)

### **1. Clean Build (Remove old build files)**

```bash
idf.py fullclean
```

**What it does:**
- Removes all build artifacts
- Clears CMake cache
- Fresh start for next build

---

### **2. Build Firmware**

```bash
idf.py build
```

**What it does:**
- Compiles all source code
- Links object files
- Creates firmware binary (build/esp32s3_michael_usb_host.bin)

**Output:**
```
Compiling C files...
Linking CXX executable...
✓ Build complete
```

---

### **3. Build + Monitor (Build and see serial output)**

```bash
idf.py build monitor
```

**What it does:**
- Builds firmware
- Automatically opens serial monitor
- Shows real-time logs from board

---

## 📤 Flash Commands (VS Code Terminal)

### **Flash to COM14 (GPIO38 - Espressif Board)**

```bash
idf.py -p COM14 flash
```

**What it does:**
- Detects COM14 port
- Erases flash memory
- Writes firmware to board
- Verifies write

**Expected Output:**
```
Connecting to COM14...
Chip is ESP32-S3
MAC: 98:a3:16:f0:6c:64
Flashing...
✓ Flash complete
```

---

### **Flash to COM11 (GPIO48 - EWeAct Board)**

```bash
idf.py -p COM11 flash
```

**What it does:**
- Detects COM11 port
- Erases flash memory
- Writes firmware to board
- Verifies write

**Expected Output:**
```
Connecting to COM11...
Chip is ESP32-S3
MAC: 48:ca:43:af:1e:40
Flashing...
✓ Flash complete
```

---

### **Flash to BOTH Boards (One after another)**

```bash
idf.py -p COM14 flash && idf.py -p COM11 flash
```

**What it does:**
- Flashes COM14 first
- Waits for completion
- Then flashes COM11
- Both boards get same firmware

**Expected Output:**
```
Flashing COM14...
✓ Flash complete
Flashing COM11...
✓ Flash complete
```

---

## 📊 Monitor Commands (VS Code Terminal)

### **Monitor COM14 (Espressif Board)**

```bash
idf.py -p COM14 monitor
```

**What it does:**
- Opens serial monitor for COM14
- Shows real-time logs
- Press Ctrl+] to exit

**Expected Output:**
```
I (0) cpu_start: Starting scheduler on APP CPU.
I (board_pins): Initializing board pins...
I (board_pins): Chip MAC address: 98:a3:16:f0:6c:64
I (board_pins): ✓ MAC matched: Espressif DevKitC-1-N8R8 → GPIO 38
I (board_pins): Board: Espressif DevKitC-1-N8R8
I (board_pins): LED GPIO: 38
```

---

### **Monitor COM11 (EWeAct Board)**

```bash
idf.py -p COM11 monitor
```

**What it does:**
- Opens serial monitor for COM11
- Shows real-time logs
- Press Ctrl+] to exit

**Expected Output:**
```
I (0) cpu_start: Starting scheduler on APP CPU.
I (board_pins): Initializing board pins...
I (board_pins): Chip MAC address: 48:ca:43:af:1e:40
I (board_pins): ✓ MAC matched: EWeAct ESP32-S3-DevKitC-1 → GPIO 48
I (board_pins): Board: EWeAct ESP32-S3-DevKitC-1
I (board_pins): LED GPIO: 48
```

---

## 🚀 Complete Workflow (Build + Flash + Monitor)

### **Workflow 1: Build → Flash COM14 → Monitor COM14**

```bash
# Step 1: Clean build
idf.py fullclean

# Step 2: Build firmware
idf.py build

# Step 3: Flash to COM14
idf.py -p COM14 flash

# Step 4: Monitor COM14
idf.py -p COM14 monitor
```

---

### **Workflow 2: Build → Flash BOTH → Monitor COM14**

```bash
# Step 1: Clean build
idf.py fullclean

# Step 2: Build firmware
idf.py build

# Step 3: Flash to both boards
idf.py -p COM14 flash && idf.py -p COM11 flash

# Step 4: Monitor COM14
idf.py -p COM14 monitor
```

---

### **Workflow 3: Build → Flash BOTH → Monitor BOTH (Two terminals)**

**Terminal 1:**
```bash
idf.py fullclean && idf.py build && idf.py -p COM14 flash && idf.py -p COM14 monitor
```

**Terminal 2 (Open new terminal):**
```bash
idf.py -p COM11 monitor
```

---

## 🛠️ Quick Reference Commands

| Task | Command |
|------|---------|
| **Clean build** | `idf.py fullclean` |
| **Build** | `idf.py build` |
| **Flash COM14** | `idf.py -p COM14 flash` |
| **Flash COM11** | `idf.py -p COM11 flash` |
| **Flash BOTH** | `idf.py -p COM14 flash && idf.py -p COM11 flash` |
| **Monitor COM14** | `idf.py -p COM14 monitor` |
| **Monitor COM11** | `idf.py -p COM11 monitor` |
| **Build + Flash COM14** | `idf.py build -p COM14 flash` |
| **Build + Flash BOTH** | `idf.py build && idf.py -p COM14 flash && idf.py -p COM11 flash` |

---

## 📝 Step-by-Step: First Time Build & Flash

### **Step 1: Open VS Code Terminal**
```
Ctrl + ` (backtick)
```

### **Step 2: Navigate to project**
```bash
cd c:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host
```

### **Step 3: Clean build**
```bash
idf.py fullclean
```

### **Step 4: Build firmware**
```bash
idf.py build
```

**Wait for:** `✓ Build complete`

### **Step 5: Flash to COM14**
```bash
idf.py -p COM14 flash
```

**Wait for:** `✓ Flash complete`

### **Step 6: Monitor COM14**
```bash
idf.py -p COM14 monitor
```

**You should see:**
```
I (board_pins): Chip MAC address: 98:a3:16:f0:6c:64
I (board_pins): ✓ MAC matched: Espressif DevKitC-1-N8R8 → GPIO 38
I (board_pins): LED GPIO: 38
```

### **Step 7: Exit monitor**
```
Press Ctrl + ]
```

### **Step 8: Flash to COM11**
```bash
idf.py -p COM11 flash
```

### **Step 9: Monitor COM11**
```bash
idf.py -p COM11 monitor
```

**You should see:**
```
I (board_pins): Chip MAC address: 48:ca:43:af:1e:40
I (board_pins): ✓ MAC matched: EWeAct ESP32-S3-DevKitC-1 → GPIO 48
I (board_pins): LED GPIO: 48
```

---

## ⚠️ Troubleshooting

### **Error: "Port COM14 not found"**
- Check USB cable connection
- Verify board is powered on
- Check Device Manager for COM port

### **Error: "Failed to connect to ESP32"**
- Press BOOT button on board
- Try again

### **Error: "Timeout waiting for packet header"**
- Board may be in wrong mode
- Disconnect and reconnect USB
- Try again

---

## 🎯 NO GPIO CHANGES NEEDED!

**Important:** You do NOT need to manually change GPIO38 to GPIO48!

The firmware automatically:
1. Reads MAC address from chip
2. Detects which board it is
3. Sets correct GPIO (38 or 48)
4. Initializes LED on correct pin

**Same firmware works on BOTH boards!** ✅

---

## 📊 Expected Results

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

## ✅ Ready to Build & Flash?

**Next Steps:**
1. Open VS Code terminal (Ctrl + `)
2. Run: `idf.py fullclean && idf.py build`
3. Flash to COM14: `idf.py -p COM14 flash`
4. Monitor: `idf.py -p COM14 monitor`
5. Observe serial output
6. Repeat for COM11

**Let's build Phase 1: LED Control! 🚀**

