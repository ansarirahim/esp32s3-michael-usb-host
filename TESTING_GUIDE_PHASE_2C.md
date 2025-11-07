# Testing Guide: Phase 2c - File Read/Write Operations

## Overview

This guide provides step-by-step instructions for testing Phase 2c file read/write operations on the ESP32-S3 USB Host Automator.

---

## Prerequisites

### Hardware Requirements
- ✅ ESP32-S3 development board (EWeAct ESP32-S3-DevKitC-1 or Espressif DevKitC-1-N8R8)
- ✅ USB cable (for programming and power)
- ✅ USB flash drive (FAT32 formatted, **NOT write-protected**)
- ✅ Computer with PuTTY or serial monitor

### Software Requirements
- ✅ ESP-IDF v5.5.1 installed
- ✅ Phase 2c firmware flashed to board
- ✅ PuTTY or serial monitor (115200 baud)

### USB Drive Requirements ⚠️ **IMPORTANT**

**File System**:
- ✅ **FAT32** (recommended)
- ✅ FAT16 (works)
- ❌ **NOT exFAT** (not supported)
- ❌ **NOT NTFS** (not supported)

**Write Protection**:
- ✅ **Write-protection switch UNLOCKED** (if present)
- ✅ **NOT read-only** file system
- ✅ **Writable** on computer

**Test File**:
- ✅ Create file named **ANSARI~1.TXT** (or any .TXT file)
- ✅ File should contain text (e.g., "Hello from USB drive")
- ✅ File size: 50-100 bytes recommended

---

## USB Drive Preparation

### Step 1: Format USB Drive

**On Windows**:
1. Insert USB drive
2. Right-click drive → **Format**
3. File system: **FAT32**
4. Allocation unit size: **Default**
5. Volume label: **TEST** (optional)
6. Click **Start**

**On Linux**:
```bash
sudo mkfs.vfat -F 32 /dev/sdX1
```

**On macOS**:
```bash
diskutil eraseDisk FAT32 TEST /dev/diskX
```

### Step 2: Create Test File

**Option 1: Using Notepad (Windows)**:
1. Open Notepad
2. Type some text:
   ```
   Hello from USB drive
   This is a test file
   ESP32-S3 will read this
   ```
3. Save as **test.txt** on USB drive

**Option 2: Using Command Line**:
```bash
echo "Hello from USB drive" > /path/to/usb/test.txt
```

### Step 3: Verify Write Protection

**Physical Check**:
- Look for write-protection switch on USB drive
- If present, ensure it's in **UNLOCKED** position

**Software Check**:
1. Try creating a new file on USB drive
2. Try deleting a file on USB drive
3. If both work, drive is writable ✅

---

## Flashing Firmware

### Step 1: Close PuTTY
If PuTTY is open, close it to free up the COM port.

### Step 2: Flash Firmware

**Using ESP-IDF**:
```bash
cd /path/to/esp32s3-michael-usb-host
idf.py -p COM11 flash
```

**Expected Output**:
```
Chip is ESP32-S3 (QFN56) (revision v0.2)
...
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
Done
```

### Step 3: Verify Flash Success
- ✅ No errors in output
- ✅ "Hash of data verified" messages
- ✅ "Done" at the end

---

## Running Tests

### Step 1: Open Serial Monitor

**Using PuTTY**:
1. Open PuTTY
2. Connection type: **Serial**
3. Serial line: **COM11** (or your port)
4. Speed: **115200**
5. Click **Open**

**Using idf.py monitor**:
```bash
idf.py -p COM11 monitor
```

### Step 2: Reset Board
Press the **RESET** button on the ESP32-S3 board.

### Step 3: Watch Boot Sequence

**Expected Output**:
```
I (344) app: ESP32-S3 USB Host Automator
I (524) led: LED control initialized successfully on GPIO 48
I (594) usb_host: ✓ USB Host initialized successfully
I (22684) app: Phase 2c: File Operations - READY FOR TEST ✓
I (22694) app: Insert USB drive to test file operations
I (22704) app: Test sequence: File Listing -> Read File -> Write File -> Safe Eject
```

**LED State**: Green slow blink (IDLE)

### Step 4: Insert USB Drive

Insert the prepared USB drive into the ESP32-S3 USB port.

**Expected Output**:
```
I (984) usb_host: New USB device connected (address: 1)
I (984) usb_host: MSC device connected
I (994) usb_host: MSC device installed successfully
I (1004) usb_host: USB drive mounted at /usb
```

**LED State**: Cyan fast blink (PREPARE)

### Step 5: Watch Automated Tests

The tests will run automatically in sequence:

#### Test 1: File Listing (0 seconds)

**Expected Output**:
```
I (23704) app: USB Drive Mounted - Listing Files
I (23714) app: [FILE] test.txt (XX bytes)
I (23734) app: Total files: X
I (23744) app: ✓ TEST PASSED: USB MSC file listing
```

**LED State**: Cyan (PREPARE)

#### Test 2: File Read (3 seconds after listing)

**Expected Output**:
```
I (26744) app: Testing File Read...
I (26744) usb_host: Reading file: /usb/test.txt
I (26754) usb_host: ✓ Read XX bytes from test.txt
I (26754) app: File contents (XX bytes):
I (26754) app: ---
I (26754) app: [your file contents displayed here]
I (26764) app: ---
I (26764) app: ✓ TEST PASSED: File read successful
```

**LED State**: Cyan (PREPARE)

#### Test 3: File Write (6 seconds after listing)

**Expected Output (SUCCESS)**:
```
I (29774) app: Testing File Write...
I (29774) usb_host: Writing file: /usb/ESP32TEST.TXT (72 bytes)
I (29784) usb_host: ✓ Wrote 72 bytes to ESP32TEST.TXT
I (29784) app: ✓ TEST PASSED: File write successful
I (29784) usb_host: Reading file: /usb/ESP32TEST.TXT
I (29794) app: Verification read (72 bytes):
I (29794) app: ---
I (29794) app: ESP32-S3 USB Host Test
I (29794) app: Phase 2c: File Operations
I (29794) app: Date: November 8, 2025
I (29804) app: ---
I (29804) app: ✓ TEST PASSED: File write verification successful
```

**Expected Output (FAILURE - Write Protected)**:
```
I (29774) app: Testing File Write...
I (29774) usb_host: Writing file: /usb/ESP32TEST.TXT (72 bytes)
E (29774) usb_host: Failed to open file for writing: /usb/ESP32TEST.TXT
E (29784) app: ✗ TEST FAILED: File write failed
```

**LED State**: Cyan (PREPARE)

#### Test 4: Safe Eject (10 seconds after listing)

**Expected Output**:
```
I (33794) app: Testing Safe Eject...
I (33814) usb_host: Step 1: Syncing filesystem...
I (34024) usb_host: ✓ Filesystem sync delay completed
I (34024) usb_host: Step 2: Unmounting VFS...
I (34024) usb_host: ✓ VFS unmounted
I (34024) usb_host: Step 3: Uninstalling MSC device...
I (34034) usb_host: Step 4: Closing USB device...
I (34044) usb_host: ✓ Safe Eject: COMPLETE
I (34054) usb_host: USB drive can now be safely removed
I (36064) app: ✓ TEST PASSED: Safe eject successful
```

**LED Sequence**:
- Magenta blink (SYNC) - 200ms
- Green solid (SUCCESS) - 2 seconds
- Green slow blink (IDLE) - continuous

### Step 6: Remove USB Drive

After seeing "USB drive can now be safely removed", you can safely remove the USB drive.

### Step 7: Verify Written File

1. Insert USB drive into computer
2. Open USB drive
3. Look for **ESP32TEST.TXT** file
4. Open file and verify contents:
   ```
   ESP32-S3 USB Host Test
   Phase 2c: File Operations
   Date: November 8, 2025
   ```

---

## Troubleshooting

### Problem 1: File Write Fails

**Symptom**:
```
E (29774) usb_host: Failed to open file for writing: /usb/ESP32TEST.TXT
E (29784) app: ✗ TEST FAILED: File write failed
```

**Solutions**:

1. **Check Write Protection Switch**:
   - Look for physical switch on USB drive
   - Move switch to UNLOCKED position
   - Retest

2. **Check File System**:
   - Verify USB drive is FAT32 (not exFAT or NTFS)
   - Reformat as FAT32 if needed
   - Retest

3. **Check Drive on Computer**:
   - Try creating a file on USB drive using computer
   - If fails, drive is write-protected or damaged
   - Use different USB drive

4. **Check Free Space**:
   - Ensure USB drive has free space
   - Delete unnecessary files if needed

### Problem 2: USB Drive Not Detected

**Symptom**:
```
I (22694) app: Insert USB drive to test file operations
[No detection message after 10 seconds]
```

**Solutions**:

1. **Check USB Connection**:
   - Ensure USB drive is fully inserted
   - Try different USB drive
   - Check USB port on ESP32-S3

2. **Check Power**:
   - Some USB drives require more power
   - Try powered USB hub
   - Try smaller capacity USB drive

3. **Check USB Drive Format**:
   - Ensure FAT32 format
   - Try reformatting

### Problem 3: File Read Fails

**Symptom**:
```
E (26744) usb_host: Failed to open file for reading: /usb/test.txt
```

**Solutions**:

1. **Check File Exists**:
   - Verify file is on USB drive
   - Check file name matches exactly
   - File names are case-sensitive on some systems

2. **Check File Path**:
   - File should be in root directory of USB drive
   - Not in subdirectory

3. **Check File System**:
   - Ensure FAT32 format
   - Check for file system corruption

### Problem 4: LED Not Changing

**Symptom**:
LED stays in one state or doesn't change colors.

**Solutions**:

1. **Check LED Connection**:
   - Verify WS2812B LED is connected to correct GPIO
   - GPIO 48 for EWeAct board
   - GPIO 38 for Espressif board

2. **Check Power**:
   - LED requires 5V power
   - Check power supply

3. **Check Code**:
   - Verify LED initialization successful in logs
   - Check for LED errors in boot sequence

---

## Expected Test Results

### Success Criteria

| Test | Expected Result |
|------|-----------------|
| USB Detection | Device detected in < 2 seconds |
| File Listing | All files listed correctly |
| File Read | File contents displayed correctly |
| File Write | ESP32TEST.TXT created on USB drive |
| File Verification | Written file contents match expected |
| Safe Eject | All 4 steps complete successfully |
| LED States | All states display correctly |

### Performance Targets

| Metric | Target | Acceptable |
|--------|--------|------------|
| USB Detection | < 1s | < 2s |
| File Listing | < 50ms | < 100ms |
| File Read | < 50ms | < 100ms |
| File Write | < 50ms | < 100ms |
| Safe Eject | < 300ms | < 500ms |

---

## Test Checklist

Before starting tests:
- [ ] USB drive formatted as FAT32
- [ ] Write-protection switch unlocked (if present)
- [ ] Test file created on USB drive
- [ ] Firmware flashed successfully
- [ ] PuTTY/serial monitor open at 115200 baud
- [ ] Board reset and ready

During tests:
- [ ] USB detection successful
- [ ] File listing shows all files
- [ ] File read displays correct contents
- [ ] File write creates ESP32TEST.TXT
- [ ] File verification shows correct contents
- [ ] Safe eject completes all 4 steps
- [ ] LED shows all states correctly

After tests:
- [ ] USB drive removed safely
- [ ] ESP32TEST.TXT exists on USB drive
- [ ] File contents match expected
- [ ] All test logs captured
- [ ] Test results documented

---

## Next Steps

After successful Phase 2c testing:

1. **Document Results**:
   - Save complete PuTTY log
   - Take photos of LED states
   - Verify ESP32TEST.TXT on computer
   - Create test results document

2. **Commit and Push**:
   - Commit test results to Git
   - Push to GitHub
   - Update project status

3. **Move to Phase 3**:
   - Phase 3a: Partition Detection & Deletion
   - Phase 3b: Partition Creation & Formatting
   - Phase 3c: File Copy from Internal Storage
   - Phase 3d: Full Automation Loop

---

**Guide Version**: 1.0  
**Last Updated**: November 8, 2025  
**Author**: Abdul Raheem Ansari

