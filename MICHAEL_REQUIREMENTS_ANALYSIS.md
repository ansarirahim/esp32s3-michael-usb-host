# Michael Steinmann Requirements Analysis

**Date:** November 8, 2025  
**Project:** ESP32-S3 USB Host Automator  
**Client:** Michael Steinmann

---

## Michael's Requirements (Received 12:13 AM)

### **Step 1: Prepare USB Drive (Two Options)**

#### **Option A:**
1. Delete all files on the attached memory stick
2. Change the FAT label (fatlabel) of the memory stick

#### **Option B (Preferred):**
1. Delete the complete partition of the attached memory stick
2. Create a new partition and format with the correct label name

### **Step 2: Copy Files**
1. Copy all files to the FAT partition
2. Sync the filesystem
3. Eject the USB device

### **Step 3: Repeat**
1. Wait for new USB device
2. Repeat from Step 1

### **Additional Requirements:**
- **Label name** should be read from a file (e.g., `fatlabel.txt`) that does NOT get copied to the memory stick
- **RGB LED** should visualize the current step

---

## Current Implementation Status

### ✅ **COMPLETED Features**

#### **Phase 1: LED Control**
- ✅ WS2812B RGB LED driver with RMT
- ✅ 6-state LED state machine:
  - `LED_STATE_IDLE` - Green slow blink (waiting for USB)
  - `LED_STATE_PREPARE` - Cyan fast blink (preparing USB drive)
  - `LED_STATE_COPY` - Yellow blink (copying files)
  - `LED_STATE_SYNC` - Magenta blink (syncing filesystem)
  - `LED_STATE_SUCCESS` - Green solid 2s (operation complete)
  - `LED_STATE_ERROR` - Red fast blink (error occurred)
- ✅ Board auto-detection by MAC address (COM11 vs COM14)

#### **Phase 2a: USB Host Initialization**
- ✅ ESP-IDF native USB Host library
- ✅ USB device detection and connection handling

#### **Phase 2b: USB MSC Driver**
- ✅ USB Mass Storage Class driver (`espressif/usb_host_msc` v1.1.4)
- ✅ VFS mounting at `/usb`
- ✅ FAT filesystem support

#### **Phase 2c: File Operations**
- ✅ File read/write operations
- ✅ File verification

#### **Phase 2d: Safe Eject**
- ✅ 4-step safe eject process:
  1. Sync filesystem
  2. Unmount VFS
  3. Uninstall MSC device
  4. Close USB device
- ✅ LED visual feedback (SYNC → SUCCESS → IDLE)

#### **Phase 3a: Partition Detection**
- ✅ Low-level sector access (SCSI READ(10)/WRITE(10))
- ✅ MBR detection and parsing
- ✅ Partition table reading
- ✅ Partition deletion capability

#### **Phase 3b: Partition Creation & Formatting**
- ✅ Drive capacity detection
- ✅ MBR partition table creation
- ✅ FAT32 formatting
- ✅ **Volume label support** ("ESP32S3" hardcoded)

#### **Phase 3c: File Copy from Internal Storage**
- ✅ SPIFFS internal storage support
- ✅ Sample file creation (5 files)
- ✅ File copy functions (single file and all files)
- ✅ Auto-format feature (formats drive if mount fails)
- ✅ LED visual feedback (COPY → SUCCESS → IDLE)

---

## Gap Analysis: What's Missing vs Michael's Requirements

### ❌ **MISSING: Step 1 - Option A (Delete Files + Change Label)**

**Michael's Requirement:**
- Delete all files on the memory stick
- Change the FAT label

**Current Status:**
- ❌ No function to delete all files from USB drive
- ❌ No function to change FAT label on existing filesystem
- ✅ We have partition deletion (Phase 3a)
- ✅ We have formatting with label (Phase 3b)

**What We Need:**
1. Function to enumerate and delete all files/directories on USB drive
2. Function to change FAT volume label without reformatting

---

### ✅ **IMPLEMENTED: Step 1 - Option B (Delete Partition + Create New)**

**Michael's Requirement:**
- Delete the complete partition
- Create new partition and format with correct label name

**Current Status:**
- ✅ `usb_host_delete_partition()` - Deletes partition (Phase 3a)
- ✅ `usb_host_create_partition_table()` - Creates MBR partition table (Phase 3b)
- ✅ `usb_host_format_fat32()` - Formats as FAT32 with volume label (Phase 3b)
- ⚠️ **Volume label is HARDCODED as "ESP32S3"** (not read from file)

**What We Need:**
1. ✅ Already have partition deletion
2. ✅ Already have partition creation
3. ✅ Already have FAT32 formatting
4. ❌ Need to read label from `fatlabel.txt` instead of hardcoding

---

### ⚠️ **PARTIAL: Step 2 - Copy Files + Sync + Eject**

**Michael's Requirement:**
1. Copy all files to the FAT partition
2. Sync the filesystem
3. Eject the USB device

**Current Status:**
- ✅ `usb_host_copy_all_files()` - Copies all files from SPIFFS to USB (Phase 3c)
- ✅ `usb_host_sync_filesystem()` - Syncs filesystem (Phase 2d)
- ✅ `usb_host_safe_eject()` - Safe eject with 4-step process (Phase 2d)
- ⚠️ **Files are copied FROM SPIFFS** (internal storage)
- ❌ **No way to specify which files to copy** (currently copies all SPIFFS files)

**What We Need:**
1. ✅ Already have file copy
2. ✅ Already have sync
3. ✅ Already have eject
4. ❌ Need to specify source directory for files to copy (not just SPIFFS)
5. ❌ Need to exclude `fatlabel.txt` from being copied

---

### ❌ **MISSING: Step 3 - Wait for New Device + Repeat**

**Michael's Requirement:**
- Wait for new USB device
- Repeat from Step 1

**Current Status:**
- ✅ USB device detection works (Phase 2a)
- ✅ USB device connection event handling works
- ❌ **No automatic loop/state machine** to repeat the process
- ❌ **Current implementation is TEST-BASED** (runs tests once, then stops)

**What We Need:**
1. State machine to handle the complete workflow:
   - State 1: IDLE (waiting for USB device)
   - State 2: PREPARE (delete partition + create + format)
   - State 3: COPY (copy files)
   - State 4: SYNC (sync filesystem)
   - State 5: EJECT (safe eject)
   - State 6: SUCCESS (show success, wait for removal)
   - State 7: REPEAT (go back to State 1)

---

### ❌ **MISSING: Read Label from File**

**Michael's Requirement:**
- Label name should be read from a file (e.g., `fatlabel.txt`)
- This file should NOT be copied to the memory stick

**Current Status:**
- ❌ Volume label is HARDCODED as "ESP32S3" in `usb_host_format_fat32()`
- ❌ No function to read label from file
- ❌ No file filtering to exclude `fatlabel.txt` from copy

**What We Need:**
1. Function to read label from `fatlabel.txt` in SPIFFS
2. Pass label as parameter to `usb_host_format_fat32()`
3. Modify `usb_host_copy_all_files()` to exclude `fatlabel.txt`

---

### ✅ **IMPLEMENTED: RGB LED Visualization**

**Michael's Requirement:**
- RGB LED should visualize the current step

**Current Status:**
- ✅ LED states already defined and working:
  - `LED_STATE_IDLE` - Green slow blink (waiting)
  - `LED_STATE_PREPARE` - Cyan fast blink (preparing - **can use for Step 1**)
  - `LED_STATE_COPY` - Yellow blink (copying - **Step 2.1**)
  - `LED_STATE_SYNC` - Magenta blink (syncing - **Step 2.2**)
  - `LED_STATE_SUCCESS` - Green solid (success - **after Step 2.3**)
  - `LED_STATE_ERROR` - Red fast blink (error)

**Mapping to Michael's Steps:**
- Step 1 (Prepare): `LED_STATE_PREPARE` (Cyan fast blink) ✅
- Step 2.1 (Copy): `LED_STATE_COPY` (Yellow blink) ✅
- Step 2.2 (Sync): `LED_STATE_SYNC` (Magenta blink) ✅
- Step 2.3 (Eject): `LED_STATE_SYNC` (Magenta blink) ✅
- Success: `LED_STATE_SUCCESS` (Green solid) ✅
- Waiting: `LED_STATE_IDLE` (Green slow blink) ✅
- Error: `LED_STATE_ERROR` (Red fast blink) ✅

---

## Summary: What We Have vs What We Need

### ✅ **We Already Have (80% Complete)**

1. ✅ USB Host initialization and device detection
2. ✅ USB MSC driver with VFS mounting
3. ✅ Partition deletion (Phase 3a)
4. ✅ Partition creation (Phase 3b)
5. ✅ FAT32 formatting with volume label (Phase 3b)
6. ✅ File copy from internal storage (Phase 3c)
7. ✅ Filesystem sync (Phase 2d)
8. ✅ Safe eject (Phase 2d)
9. ✅ RGB LED state machine with all required states (Phase 1)
10. ✅ Auto-format feature (Phase 3c)

### ❌ **What We Still Need (20% Remaining)**

1. ❌ **Read volume label from `fatlabel.txt`** (instead of hardcoding "ESP32S3")
2. ❌ **Exclude `fatlabel.txt` from file copy** (file filtering)
3. ❌ **Main workflow state machine** (automatic loop: prepare → copy → sync → eject → repeat)
4. ❌ **Option A implementation** (delete files + change label without reformatting)
5. ❌ **Configurable source directory** for files to copy (not just SPIFFS)

---

## Recommended Next Steps (Phase 4)

### **Phase 4a: Label Configuration**
- Read volume label from `/spiffs/fatlabel.txt`
- Modify `usb_host_format_fat32()` to accept label as parameter
- Add file filtering to exclude `fatlabel.txt` from copy

### **Phase 4b: Main Workflow State Machine**
- Implement automatic workflow loop
- State transitions: IDLE → PREPARE → COPY → SYNC → EJECT → SUCCESS → IDLE
- LED state updates for each step
- Error handling and recovery

### **Phase 4c: Option A Implementation (Optional)**
- Function to delete all files on USB drive
- Function to change FAT label without reformatting
- User can choose Option A or Option B

---

## Conclusion

**Current Progress: 80% Complete**

We have successfully implemented:
- ✅ All low-level USB operations
- ✅ Partition management (delete, create, format)
- ✅ File operations (copy, sync, eject)
- ✅ LED visual feedback
- ✅ Michael's **Option B (Preferred)** - Delete partition + create new

**What's Missing:**
- ❌ Read label from file (instead of hardcoding)
- ❌ Main workflow state machine (automatic loop)
- ❌ File filtering (exclude `fatlabel.txt`)
- ❌ Option A (delete files + change label)

**Recommendation:**
Proceed with **Phase 4a** and **Phase 4b** to complete Michael's requirements.

---

**Author:** Abdul Raheem Ansari  
**Project:** ESP32-S3 USB Host Automator  
**Client:** Michael Steinmann  
**Date:** November 8, 2025

