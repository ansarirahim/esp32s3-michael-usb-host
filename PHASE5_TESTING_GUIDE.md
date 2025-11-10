# Phase 5: USB Mode Switching - Testing Guide

**Date:** November 8, 2025  
**Author:** Abdul Raheem Ansari  
**Project:** ESP32-S3 USB Host Automator for Michael Steinmann  
**Branch:** `feature/usb-mode-switching-v2`

---

## 🎯 **Testing Objectives**

Validate that Phase 5 USB Mode Switching implementation works correctly:
1. ✅ Triple-press button detection
2. ✅ Mode switching (Host ↔ Device)
3. ✅ LED indication during mode switching
4. ✅ Safety checks (deny switch when USB active)
5. ✅ Fail-safe behavior (revert to Host on failure)
6. ✅ Default mode (always boots into Host mode)

---

## 🔧 **Prerequisites**

### **Hardware Required:**
- ✅ ESP32-S3 DevKitC-1 board (COM14 or COM11)
- ✅ USB cable (data transfer capable)
- ✅ USB flash drive (FAT32 formatted, any size)
- ✅ PC with serial monitor

### **Software Required:**
- ✅ ESP-IDF v5.5.1 installed
- ✅ Phase 5 firmware built and flashed
- ✅ Serial monitor (idf.py monitor or PuTTY)

### **Initial State:**
- ✅ **USB port EMPTY** (no drive, no cable)
- ✅ **ESP32-S3 powered on**
- ✅ **Serial monitor connected** (115200 baud)

---

## 📋 **Test Cases**

### **Test 1: Power On (Default Mode)** ⭐ CRITICAL

**Objective:** Verify system always boots into USB Host mode

**Steps:**
1. Power on ESP32-S3 (or press RESET button)
2. Observe LED color and pattern
3. Check serial monitor output

**Expected Results:**
- ✅ **LED:** Green slow blink (500ms ON / 1500ms OFF)
- ✅ **Serial log:** "USB Mode Manager initialized successfully (mode: HOST)"
- ✅ **Serial log:** "Triple-press BOOT button to toggle USB mode (Host <-> Device)"

**Pass Criteria:**
- [ ] Green LED slow blink visible
- [ ] Serial log shows "mode: HOST"
- [ ] No errors in serial log

**Result:** ⬜ PASS / ⬜ FAIL

**Notes:**
```
_____________________________________________
```

---

### **Test 2: Triple-Press Mode Switch (Host → Device)** ⭐ CRITICAL

**Objective:** Verify triple-press button switches from Host to Device mode

**Preconditions:**
- ✅ ESP32-S3 in Host mode (green LED slow blink)
- ✅ USB port is EMPTY (no drive inserted)

**Steps:**
1. Press BOOT button (GPIO 0) **3 times** within 2 seconds
   - Press 1: Quick press and release
   - Press 2: Quick press and release (within 2 seconds of press 1)
   - Press 3: Quick press and release (within 2 seconds of press 1)
2. Observe LED color changes
3. Check serial monitor output

**Expected Results:**
- ✅ **Serial log (after each press):** "Button press detected (1/3)", "Button press detected (2/3)", "Button press detected (3/3)"
- ✅ **Serial log:** "Triple-press detected! Toggling USB mode..."
- ✅ **LED:** Orange fast blink (100ms ON / 100ms OFF) - mode switching in progress
- ✅ **Serial log:** "Mode switch complete: DEVICE"
- ✅ **LED:** Blue slow blink (500ms ON / 1500ms OFF) - Device mode

**Pass Criteria:**
- [ ] All 3 button presses detected
- [ ] Orange LED fast blink during switching
- [ ] Blue LED slow blink after switching
- [ ] Serial log shows "Mode switch complete: DEVICE"
- [ ] No errors in serial log

**Result:** ⬜ PASS / ⬜ FAIL

**Notes:**
```
_____________________________________________
```

---

### **Test 3: Triple-Press Mode Switch (Device → Host)** ⭐ CRITICAL

**Objective:** Verify triple-press button switches from Device to Host mode

**Preconditions:**
- ✅ ESP32-S3 in Device mode (blue LED slow blink)
- ✅ USB port is EMPTY (not connected to PC)

**Steps:**
1. Press BOOT button **3 times** within 2 seconds
2. Observe LED color changes
3. Check serial monitor output

**Expected Results:**
- ✅ **Serial log:** "Triple-press detected! Toggling USB mode..."
- ✅ **LED:** Orange fast blink (100ms ON / 100ms OFF)
- ✅ **Serial log:** "Mode switch complete: HOST"
- ✅ **LED:** Green slow blink (500ms ON / 1500ms OFF)

**Pass Criteria:**
- [ ] Orange LED fast blink during switching
- [ ] Green LED slow blink after switching
- [ ] Serial log shows "Mode switch complete: HOST"
- [ ] No errors in serial log

**Result:** ⬜ PASS / ⬜ FAIL

**Notes:**
```
_____________________________________________
```

---

### **Test 4: USB Host Workflow (Insert USB Drive)** ⭐ CRITICAL

**Objective:** Verify USB Host workflow still works after Phase 5 changes

**Preconditions:**
- ✅ ESP32-S3 in Host mode (green LED slow blink)
- ✅ USB flash drive ready (FAT32 formatted)

**Steps:**
1. Insert USB flash drive into ESP32-S3 USB port
2. Wait for automated workflow to complete
3. Observe LED color changes
4. Check serial monitor output
5. Remove USB drive and check files on PC

**Expected Results:**
- ✅ **LED sequence:**
   - Cyan fast blink (preparing USB drive)
   - Yellow blink (copying files)
   - Magenta blink (syncing filesystem)
   - Green solid 2s (success)
   - Green slow blink (idle, waiting for next USB)
- ✅ **Serial log:** "Workflow: PREPARE → COPY → SYNC → EJECT → SUCCESS"
- ✅ **USB drive:** Contains copied files from SPIFFS
- ✅ **USB drive:** Volume label matches fatlabel.txt (if present)

**Pass Criteria:**
- [ ] All LED states visible in correct sequence
- [ ] Files copied successfully to USB drive
- [ ] USB drive safely ejected
- [ ] No errors in serial log

**Result:** ⬜ PASS / ⬜ FAIL

**Notes:**
```
_____________________________________________
```

---

### **Test 5: Mode Switch Denied (USB Drive Mounted)** ⭐ CRITICAL

**Objective:** Verify mode switching is denied when USB drive is mounted

**Preconditions:**
- ✅ ESP32-S3 in Host mode
- ✅ USB flash drive inserted and mounted

**Steps:**
1. Insert USB drive and wait for workflow to start
2. During file copy (yellow LED blinking), press BOOT button **3 times**
3. Observe LED color
4. Check serial monitor output
5. Verify file copy continues uninterrupted

**Expected Results:**
- ✅ **Serial log:** "Cannot switch: USB drive is mounted"
- ✅ **LED:** Red fast blink for 2 seconds (error indication)
- ✅ **LED:** Returns to previous state (yellow blink - copying continues)
- ✅ **Serial log:** File copy completes successfully
- ✅ **Mode:** Remains in Host mode (no mode switch occurred)

**Pass Criteria:**
- [ ] Red LED fast blink for 2 seconds
- [ ] Serial log shows "Cannot switch: USB drive is mounted"
- [ ] File copy continues without interruption
- [ ] No data corruption on USB drive
- [ ] Mode remains Host (green LED after workflow completes)

**Result:** ⬜ PASS / ⬜ FAIL

**Notes:**
```
_____________________________________________
```

---

### **Test 6: Incomplete Triple-Press (Timeout)** ⭐ IMPORTANT

**Objective:** Verify incomplete triple-press does not trigger mode switch

**Preconditions:**
- ✅ ESP32-S3 in Host mode (green LED slow blink)
- ✅ USB port is EMPTY

**Steps:**
1. Press BOOT button **2 times** (not 3)
2. Wait 3 seconds (triple-press window timeout)
3. Observe LED color
4. Check serial monitor output

**Expected Results:**
- ✅ **Serial log:** "Button press detected (1/3)"
- ✅ **Serial log:** "Button press detected (2/3)"
- ✅ **Serial log (after 2s timeout):** "Triple-press timeout, resetting (press_count: 2)"
- ✅ **LED:** Remains green slow blink (no mode switch)
- ✅ **Mode:** Remains in Host mode

**Pass Criteria:**
- [ ] Only 2 button presses detected
- [ ] Serial log shows timeout message
- [ ] No mode switch occurred
- [ ] LED remains green slow blink

**Result:** ⬜ PASS / ⬜ FAIL

**Notes:**
```
_____________________________________________
```

---

### **Test 7: Rapid Button Presses (Debounce Test)** ⭐ IMPORTANT

**Objective:** Verify debounce prevents false triggers from rapid presses

**Preconditions:**
- ✅ ESP32-S3 in Host mode
- ✅ USB port is EMPTY

**Steps:**
1. Press BOOT button **very rapidly** 5-10 times (faster than 150ms between presses)
2. Observe serial monitor output
3. Check if mode switch occurred

**Expected Results:**
- ✅ **Serial log:** Some presses ignored due to debounce/min inter-press interval
- ✅ **Serial log:** "Button press too fast, ignoring (inter-press: XX ms)"
- ✅ **LED:** May show orange blink briefly, but should NOT complete mode switch
- ✅ **Mode:** Remains in Host mode (or reverts to Host if partial switch attempted)

**Pass Criteria:**
- [ ] Not all rapid presses are counted
- [ ] Serial log shows "too fast" messages
- [ ] No unintended mode switch
- [ ] System remains stable

**Result:** ⬜ PASS / ⬜ FAIL

**Notes:**
```
_____________________________________________
```

---

### **Test 8: Power Cycle Persistence Test** ⭐ IMPORTANT

**Objective:** Verify system always boots into Host mode (no mode persistence)

**Preconditions:**
- ✅ ESP32-S3 powered on

**Steps:**
1. Switch to Device mode (triple-press, verify blue LED)
2. Power off ESP32-S3 (unplug USB cable)
3. Wait 5 seconds
4. Power on ESP32-S3 (plug USB cable)
5. Observe LED color
6. Check serial monitor output

**Expected Results:**
- ✅ **LED:** Green slow blink (Host mode)
- ✅ **Serial log:** "USB Mode Manager initialized successfully (mode: HOST)"
- ✅ **Mode:** Host mode (NOT Device mode)

**Pass Criteria:**
- [ ] System boots into Host mode (green LED)
- [ ] Previous mode (Device) is NOT remembered
- [ ] Serial log confirms Host mode initialization

**Result:** ⬜ PASS / ⬜ FAIL

**Notes:**
```
_____________________________________________
```

---

## 📊 **Test Results Summary**

| Test # | Test Name | Status | Notes |
|--------|-----------|--------|-------|
| 1 | Power On (Default Mode) | ⬜ PASS / ⬜ FAIL | |
| 2 | Triple-Press (Host → Device) | ⬜ PASS / ⬜ FAIL | |
| 3 | Triple-Press (Device → Host) | ⬜ PASS / ⬜ FAIL | |
| 4 | USB Host Workflow | ⬜ PASS / ⬜ FAIL | |
| 5 | Mode Switch Denied (USB Mounted) | ⬜ PASS / ⬜ FAIL | |
| 6 | Incomplete Triple-Press | ⬜ PASS / ⬜ FAIL | |
| 7 | Rapid Button Presses | ⬜ PASS / ⬜ FAIL | |
| 8 | Power Cycle Persistence | ⬜ PASS / ⬜ FAIL | |

**Total Tests:** 8  
**Passed:** ___  
**Failed:** ___  
**Pass Rate:** ____%

---

## 🐛 **Troubleshooting**

### **Issue: Button presses not detected**
**Symptoms:** No serial log messages when pressing BOOT button  
**Possible Causes:**
- Button handler not initialized
- GPIO 0 not configured correctly
- Serial monitor not connected

**Solutions:**
1. Check serial log for "Button handler initialized successfully"
2. Verify BOOT button is functional (try bootloader mode: hold BOOT + press RESET)
3. Reflash firmware

---

### **Issue: Mode switch fails (orange LED, then red LED)**
**Symptoms:** Orange fast blink → Red fast blink → Returns to previous mode  
**Possible Causes:**
- USB initialization failure
- USB PHY not stabilizing
- Insufficient delay between deinit/init

**Solutions:**
1. Check serial log for error messages
2. Power cycle ESP32-S3
3. Try mode switch again (retry logic should handle transient failures)

---

### **Issue: Mode switch denied unexpectedly**
**Symptoms:** Red LED fast blink when USB port appears empty  
**Possible Causes:**
- USB drive still mounted internally
- Workflow still active
- USB cable connected to PC

**Solutions:**
1. Wait for workflow to complete (green solid 2s)
2. Remove USB drive physically
3. Disconnect USB cable from PC (if in Device mode)
4. Wait 2 seconds after USB event before trying mode switch

---

## ✅ **Test Completion Checklist**

- [ ] All 8 test cases executed
- [ ] Test results documented in table above
- [ ] Serial logs captured for failed tests
- [ ] Issues reported (if any)
- [ ] Firmware version recorded
- [ ] Board variant tested (COM14 / COM11)
- [ ] Test date and tester name recorded

**Tested By:** _______________________  
**Date:** _______________________  
**Board:** ⬜ COM14 (GPIO38 - Espressif) / ⬜ COM11 (GPIO48 - EWeAct)  
**Firmware Version:** Phase 5 - USB Mode Switching v1.0  
**Branch:** feature/usb-mode-switching-v2

---

## 🎉 **Next Steps After Testing**

If all tests pass:
1. ✅ Create TEST_RESULTS_PHASE_5.md with detailed results
2. ✅ Commit changes to Git
3. ✅ Push to GitHub
4. ✅ Create pull request to merge into develop branch
5. ✅ Prepare delivery package for Michael

If any tests fail:
1. ❌ Document failure details
2. ❌ Debug and fix issues
3. ❌ Rebuild and retest
4. ❌ Repeat until all tests pass

---

**Good luck with testing!** 🚀

