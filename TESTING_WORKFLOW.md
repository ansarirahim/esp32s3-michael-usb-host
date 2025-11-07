# 🧪 Testing Workflow - Phase 1 Board Detection

**Project:** ESP32-S3 USB Host Automator
**Date:** November 7, 2025
**Phase:** 1 - Board Detection Testing
**Status:** READY TO TEST

---

## 🎯 Testing Objectives

1. ✅ Build firmware successfully
2. ✅ Flash to COM14 (GPIO38 - Espressif)
3. ✅ Verify board detection on COM14
4. ✅ Flash to COM11 (GPIO48 - EWeAct)
5. ✅ Verify board detection on COM11
6. ✅ Confirm GPIO auto-detection works

---

## 📋 Step-by-Step Testing

### **STEP 1: Open VS Code Terminal**

```
Press: Ctrl + ` (backtick)
```

**Expected:** Terminal opens at bottom of VS Code

---

### **STEP 2: Navigate to Project**

```bash
cd c:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host
```

**Expected:** Terminal shows project path

---

### **STEP 3: Clean Build**

```bash
build.bat clean
```

**Expected Output:**
```
Cleaning build...
Executing: idf.py fullclean
...
✓ Build clean complete
```

**Wait for:** Command to complete (1-2 minutes)

---

### **STEP 4: Build Firmware**

```bash
build.bat build
```

**Expected Output:**
```
Building firmware...
Executing: idf.py build
Compiling C files...
Linking CXX executable...
✓ Build complete
```

**Wait for:** `✓ Build complete` message

**If error:** Run `build.bat clean` and try again

---

### **STEP 5: Flash to COM14 (Espressif - GPIO38)**

```bash
build.bat flash-com14
```

**Expected Output:**
```
Flashing to COM14 (GPIO38 - Espressif)...
Executing: idf.py -p COM14 flash
Connecting to COM14...
Chip is ESP32-S3
MAC: 98:a3:16:f0:6c:64
Flashing...
✓ Flash complete
```

**Wait for:** `✓ Flash complete` message

**If error:** 
- Check USB cable
- Verify COM14 in Device Manager
- Press BOOT button on board
- Try again

---

### **STEP 6: Monitor COM14 Serial Output**

```bash
build.bat monitor-com14
```

**Expected Output:**
```
Monitoring COM14 (GPIO38 - Espressif)...
Executing: idf.py -p COM14 monitor
...
I (0) cpu_start: Starting scheduler on APP CPU.
I (board_pins): Initializing board pins...
I (board_pins): Chip MAC address: 98:a3:16:f0:6c:64
I (board_pins): ✓ MAC matched: Espressif DevKitC-1-N8R8 → GPIO 38
I (board_pins): Board: Espressif DevKitC-1-N8R8
I (board_pins): LED GPIO: 38
I (app): Application ready
```

**Verify:**
- ✅ MAC address: 98:a3:16:f0:6c:64
- ✅ Board name: Espressif DevKitC-1-N8R8
- ✅ GPIO: 38

**Record this output!** Take a screenshot or note it.

---

### **STEP 7: Exit Monitor**

```
Press: Ctrl + ]
```

**Expected:** Monitor exits and returns to terminal prompt

---

### **STEP 8: Flash to COM11 (EWeAct - GPIO48)**

```bash
build.bat flash-com11
```

**Expected Output:**
```
Flashing to COM11 (GPIO48 - EWeAct)...
Executing: idf.py -p COM11 flash
Connecting to COM11...
Chip is ESP32-S3
MAC: 48:ca:43:af:1e:40
Flashing...
✓ Flash complete
```

**Wait for:** `✓ Flash complete` message

---

### **STEP 9: Monitor COM11 Serial Output**

```bash
build.bat monitor-com11
```

**Expected Output:**
```
Monitoring COM11 (GPIO48 - EWeAct)...
Executing: idf.py -p COM11 monitor
...
I (0) cpu_start: Starting scheduler on APP CPU.
I (board_pins): Initializing board pins...
I (board_pins): Chip MAC address: 48:ca:43:af:1e:40
I (board_pins): ✓ MAC matched: EWeAct ESP32-S3-DevKitC-1 → GPIO 48
I (board_pins): Board: EWeAct ESP32-S3-DevKitC-1
I (board_pins): LED GPIO: 48
I (app): Application ready
```

**Verify:**
- ✅ MAC address: 48:ca:43:af:1e:40
- ✅ Board name: EWeAct ESP32-S3-DevKitC-1
- ✅ GPIO: 48

**Record this output!** Take a screenshot or note it.

---

### **STEP 10: Exit Monitor**

```
Press: Ctrl + ]
```

**Expected:** Monitor exits and returns to terminal prompt

---

## ✅ Testing Checklist

### **COM14 (Espressif - GPIO38):**
- [ ] Build successful
- [ ] Flash successful
- [ ] Serial output shows MAC: 98:a3:16:f0:6c:64
- [ ] Serial output shows Board: Espressif DevKitC-1-N8R8
- [ ] Serial output shows GPIO: 38
- [ ] No errors in serial output

### **COM11 (EWeAct - GPIO48):**
- [ ] Flash successful
- [ ] Serial output shows MAC: 48:ca:43:af:1e:40
- [ ] Serial output shows Board: EWeAct ESP32-S3-DevKitC-1
- [ ] Serial output shows GPIO: 48
- [ ] No errors in serial output

---

## 📊 Expected Results

### **COM14 Output:**
```
I (board_pins): Chip MAC address: 98:a3:16:f0:6c:64
I (board_pins): ✓ MAC matched: Espressif DevKitC-1-N8R8 → GPIO 38
I (board_pins): Board: Espressif DevKitC-1-N8R8
I (board_pins): LED GPIO: 38
```

### **COM11 Output:**
```
I (board_pins): Chip MAC address: 48:ca:43:af:1e:40
I (board_pins): ✓ MAC matched: EWeAct ESP32-S3-DevKitC-1 → GPIO 48
I (board_pins): Board: EWeAct ESP32-S3-DevKitC-1
I (board_pins): LED GPIO: 48
```

---

## 🎯 Success Criteria

✅ **Phase 1 Board Detection Test PASSES if:**

1. Firmware builds without errors
2. Firmware flashes to COM14 successfully
3. COM14 serial output shows:
   - MAC: 98:a3:16:f0:6c:64
   - Board: Espressif DevKitC-1-N8R8
   - GPIO: 38
4. Firmware flashes to COM11 successfully
5. COM11 serial output shows:
   - MAC: 48:ca:43:af:1e:40
   - Board: EWeAct ESP32-S3-DevKitC-1
   - GPIO: 48
6. Both boards auto-detect correct GPIO
7. Same firmware works on both boards

---

## ⚠️ Troubleshooting

### **Build Fails:**
```bash
build.bat clean
build.bat build
```

### **Flash Fails:**
- Check USB cable connection
- Verify COM port in Device Manager
- Press BOOT button on board
- Try again

### **Monitor Shows Garbage:**
- Check baud rate (should be 115200)
- Disconnect and reconnect USB
- Try again

### **Wrong GPIO Detected:**
- Check MAC address in output
- Verify board is correct
- Check board_pins.c for MAC table

---

## 📝 Testing Report Template

**Date:** _______________
**Tester:** _______________

### **COM14 (Espressif - GPIO38):**
- Build: ✅ / ❌
- Flash: ✅ / ❌
- MAC Address: _______________________
- Board Name: _______________________
- GPIO: _______________________
- Status: ✅ PASS / ❌ FAIL

### **COM11 (EWeAct - GPIO48):**
- Flash: ✅ / ❌
- MAC Address: _______________________
- Board Name: _______________________
- GPIO: _______________________
- Status: ✅ PASS / ❌ FAIL

### **Overall Result:**
✅ PASS / ❌ FAIL

### **Notes:**
_________________________________________________
_________________________________________________

---

## 🚀 Next Steps After Testing

### **If PASS:**
1. ✅ Board detection working
2. ✅ GPIO auto-detection working
3. ⏭️ Implement Phase 1: LED Control
4. ⏭️ Implement RMT peripheral driver
5. ⏭️ Create LED state machine
6. ⏭️ Test LED on both boards

### **If FAIL:**
1. ❌ Check error messages
2. ❌ Verify board connections
3. ❌ Check MAC addresses in board_pins.c
4. ❌ Try again

---

## 📞 Quick Help

**Q: What if build fails?**
A: Run `build.bat clean` first, then `build.bat build`

**Q: What if flash fails?**
A: Check USB cable, verify COM port, press BOOT button

**Q: What if monitor shows garbage?**
A: Disconnect USB, reconnect, try again

**Q: How do I know if it's working?**
A: Check serial output for correct MAC, board name, and GPIO

---

## 🎉 Ready to Test?

**Run this command:**
```bash
build.bat clean && build.bat build && build.bat flash-both && build.bat monitor-com14
```

**Then:**
1. Observe COM14 output
2. Exit monitor (Ctrl + ])
3. Run: `build.bat monitor-com11`
4. Observe COM11 output
5. Exit monitor (Ctrl + ])

**Report back with:**
- ✅ Build successful?
- ✅ Flash to COM14 successful?
- ✅ COM14 shows GPIO 38?
- ✅ Flash to COM11 successful?
- ✅ COM11 shows GPIO 48?

**Let's test! 🚀**

