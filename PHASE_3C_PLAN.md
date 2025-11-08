# Phase 3c: File Copy from Internal Storage - Implementation Plan

## Overview

**Phase 3c** implements internal storage and file copy functionality, enabling the ESP32-S3 to:
- Store files in internal flash memory (SPIFFS filesystem)
- Copy files from internal storage to USB drive
- Verify file integrity after copy
- Support multiple files and directories

This phase builds on Phase 3a (Partition Detection) and Phase 3b (Partition Creation) and prepares for Phase 3d (Full Automation).

---

## Objectives

1. ✅ Initialize SPIFFS filesystem on internal flash
2. ✅ Create sample files in SPIFFS
3. ✅ Implement file copy from SPIFFS to USB
4. ✅ Verify file integrity (size and content)
5. ✅ Support multiple files
6. ✅ Add progress reporting
7. ✅ Add automated tests

---

## Technical Requirements

### 1. SPIFFS Filesystem

**SPIFFS** (SPI Flash File System) is a lightweight filesystem designed for embedded systems:
- **Wear leveling**: Distributes writes across flash memory
- **No directories**: Flat file structure (directories are simulated with "/" in filenames)
- **Small overhead**: Minimal RAM and flash usage
- **Reliable**: Handles power failures gracefully

**ESP-IDF SPIFFS Configuration**:
- **Partition**: Defined in `partitions.csv`
- **Mount point**: `/spiffs`
- **Max files**: 5-10 open files
- **Auto format**: Format on first mount if needed

### 2. Partition Table

Current partition table (from `partitions.csv`):
```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0xC0000,
storage,  data, spiffs,  0xD0000, 0x40000,
```

**Storage Partition**:
- **Offset**: 0xD0000 (832 KB)
- **Size**: 0x40000 (256 KB)
- **Type**: SPIFFS

### 3. File Copy Strategy

**Copy Process**:
1. Mount SPIFFS filesystem
2. List files in SPIFFS
3. For each file:
   - Open source file (SPIFFS)
   - Open destination file (USB)
   - Read chunk from source
   - Write chunk to destination
   - Repeat until EOF
   - Close both files
   - Verify file size
4. Report progress and results

**Chunk Size**: 512 bytes (matches sector size)

---

## API Design

### New Functions

```c
/**
 * @brief Initialize internal storage (SPIFFS)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t internal_storage_init(void);

/**
 * @brief Deinitialize internal storage
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t internal_storage_deinit(void);

/**
 * @brief Create sample files in internal storage
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t internal_storage_create_samples(void);

/**
 * @brief Copy a file from internal storage to USB drive
 * @param filename Filename (without path)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t copy_file_to_usb(const char* filename);

/**
 * @brief Copy all files from internal storage to USB drive
 * @param files_copied Pointer to store number of files copied (optional)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t copy_all_files_to_usb(uint32_t* files_copied);

/**
 * @brief Get internal storage mount point
 * @return Mount point string or NULL if not mounted
 */
const char* internal_storage_get_mount_point(void);
```

---

## Implementation Steps

### Step 1: Configure SPIFFS
- Verify partition table has storage partition
- Configure SPIFFS in `sdkconfig`:
  - `CONFIG_SPIFFS_MAX_PARTITIONS=1`
  - `CONFIG_SPIFFS_USE_MAGIC=y`
  - `CONFIG_SPIFFS_USE_MAGIC_LENGTH=y`

### Step 2: Initialize SPIFFS
- Create `internal_storage.c` and `internal_storage.h`
- Implement `internal_storage_init()`:
  - Configure SPIFFS
  - Mount at `/spiffs`
  - Auto-format if needed
  - Log partition info

### Step 3: Create Sample Files
- Implement `internal_storage_create_samples()`:
  - Create 3-5 sample files
  - Different sizes (small, medium, large)
  - Different content types (text, binary)
  - Store in `/spiffs/`

### Step 4: Implement File Copy
- Implement `copy_file_to_usb()`:
  - Open source file from SPIFFS
  - Open destination file on USB
  - Copy in 512-byte chunks
  - Verify file size
  - Report progress

### Step 5: Implement Batch Copy
- Implement `copy_all_files_to_usb()`:
  - List all files in SPIFFS
  - Copy each file
  - Count successes/failures
  - Report summary

### Step 6: Add Progress Reporting
- LED state changes during copy
- Log progress for each file
- Report total bytes copied

---

## Sample Files

### File 1: README.TXT (Small - 200 bytes)
```
ESP32-S3 USB Host Automator
============================

This USB drive was automatically formatted and populated by an ESP32-S3.

Project: USB Host Mode Automation
Author: Abdul Raheem Ansari
Date: November 2025

Files on this drive were copied from internal flash storage.
```

### File 2: CONFIG.INI (Small - 150 bytes)
```
[Settings]
AutoFormat=true
FileSystem=FAT32
ClusterSize=4096
VolumeLabel=ESP32-USB

[Files]
CopyAll=true
VerifyAfterCopy=true
```

### File 3: DATA.BIN (Medium - 1024 bytes)
Binary data with pattern: 0x00, 0x01, 0x02, ..., 0xFF (repeating)

### File 4: LOG.TXT (Medium - 500 bytes)
```
ESP32-S3 USB Host Automator - Operation Log
============================================

[2025-11-08 12:00:00] System initialized
[2025-11-08 12:00:01] USB drive detected
[2025-11-08 12:00:02] Partition table detected: MBR
[2025-11-08 12:00:03] Partition deleted
[2025-11-08 12:00:04] New partition created
[2025-11-08 12:00:05] Partition formatted: FAT32
[2025-11-08 12:00:06] Files copied: 4
[2025-11-08 12:00:07] Operation complete
```

### File 5: TEST.DAT (Large - 4096 bytes)
Binary data with incrementing pattern

---

## Test Plan

### Test 1: Initialize SPIFFS
- ✅ Mount SPIFFS filesystem
- ✅ Verify mount point accessible
- ✅ Check partition info

### Test 2: Create Sample Files
- ✅ Create 5 sample files in SPIFFS
- ✅ Verify files exist
- ✅ Verify file sizes

### Test 3: Copy Single File
- ✅ Copy one file from SPIFFS to USB
- ✅ Verify file exists on USB
- ✅ Verify file size matches
- ✅ Verify file content matches

### Test 4: Copy All Files
- ✅ Copy all files from SPIFFS to USB
- ✅ Verify all files exist on USB
- ✅ Verify file count matches
- ✅ Verify total bytes copied

### Test 5: Verify on PC
- ✅ Safe eject USB drive
- ✅ Insert into PC
- ✅ Verify all files present
- ✅ Verify file contents

---

## LED State Integration

| Operation | LED State | Color | Pattern |
|-----------|-----------|-------|---------|
| Initializing SPIFFS | PREPARE | Cyan | Fast blink |
| Creating sample files | PREPARE | Cyan | Fast blink |
| Copying files | COPY | Yellow | Blink |
| Verifying files | SYNC | Magenta | Blink |
| Copy complete | SUCCESS | Green | Solid 2s |
| Copy failed | ERROR | Red | Fast blink |

---

## Error Handling

### SPIFFS Errors
- **Mount failed**: Format and retry
- **File not found**: Skip and continue
- **Disk full**: Report error and stop

### USB Errors
- **Not mounted**: Report error
- **Write failed**: Retry once, then fail
- **Disk full**: Report error and stop

### Copy Errors
- **Source read failed**: Skip file
- **Destination write failed**: Delete partial file
- **Size mismatch**: Delete and retry

---

## Success Criteria

| Criterion | Target | Status |
|-----------|--------|--------|
| Initialize SPIFFS | Success | ⏸️ |
| Create sample files | 5 files | ⏸️ |
| Copy single file | Success | ⏸️ |
| Copy all files | 5 files | ⏸️ |
| Verify file sizes | 100% match | ⏸️ |
| Verify file contents | 100% match | ⏸️ |
| No crashes | 0 crashes | ⏸️ |
| Code quality | Clean, documented | ⏸️ |

---

## Expected Results

### SPIFFS Contents (Before Copy):
```
/spiffs/README.TXT (200 bytes)
/spiffs/CONFIG.INI (150 bytes)
/spiffs/DATA.BIN (1024 bytes)
/spiffs/LOG.TXT (500 bytes)
/spiffs/TEST.DAT (4096 bytes)
Total: 5 files, 5970 bytes
```

### USB Contents (After Copy):
```
/usb/README.TXT (200 bytes)
/usb/CONFIG.INI (150 bytes)
/usb/DATA.BIN (1024 bytes)
/usb/LOG.TXT (500 bytes)
/usb/TEST.DAT (4096 bytes)
Total: 5 files, 5970 bytes
```

---

## Timeline

- **Planning**: 30 minutes ✅
- **Implementation**: 2-3 hours ⏸️
- **Testing**: 1 hour ⏸️
- **Documentation**: 30 minutes ⏸️
- **Total**: 4-5 hours

---

## Dependencies

- ✅ Phase 3a: Partition Detection (COMPLETE)
- ✅ Phase 3b: Partition Creation & Formatting (COMPLETE)
- ✅ SPIFFS component (built into ESP-IDF)
- ✅ Storage partition in partition table (exists)

---

## Next Phase

**Phase 3d: Full Automation Loop**
- Detect USB drive insertion
- Delete all partitions
- Create new partition
- Format as FAT32
- Copy files from SPIFFS
- Safe eject
- LED feedback throughout process
- Complete automation with no user interaction

---

**Status**: 🚧 IN PROGRESS  
**Branch**: `feature/phase-3c-file-copy`  
**Started**: November 8, 2025

