# Phase 3b: Partition Creation & Formatting - Implementation Plan

## Overview

**Phase 3b** implements partition creation and formatting functionality, enabling the ESP32-S3 to:
- Create a new MBR partition table
- Create a single FAT32 partition
- Format the partition as FAT32
- Mount the newly created partition

This phase builds on Phase 3a (Partition Detection & Deletion) and prepares for Phase 3c (File Copy).

---

## Objectives

1. ✅ Create new MBR partition table
2. ✅ Create single FAT32 partition (using full drive capacity)
3. ✅ Format partition as FAT32
4. ✅ Mount newly created partition
5. ✅ Verify partition is writable
6. ✅ Add automated tests

---

## Technical Requirements

### 1. MBR Creation

**Master Boot Record Structure** (Sector 0, 512 bytes):
- **Offset 0x000-0x1BD** (446 bytes): Boot code (zeros for data disk)
- **Offset 0x1BE-0x1CD** (16 bytes): Partition Entry 1
- **Offset 0x1CE-0x1DD** (16 bytes): Partition Entry 2 (empty)
- **Offset 0x1DE-0x1ED** (16 bytes): Partition Entry 3 (empty)
- **Offset 0x1EE-0x1FD** (16 bytes): Partition Entry 4 (empty)
- **Offset 0x1FE-0x1FF** (2 bytes): Boot signature (0x55AA)

**Partition Entry Structure** (16 bytes):
```
Offset  Size  Description
------  ----  -----------
0x00    1     Boot indicator (0x00 = non-bootable, 0x80 = bootable)
0x01    3     CHS start address (can be 0xFFFFFF for LBA)
0x04    1     Partition type (0x0C = FAT32 LBA)
0x05    3     CHS end address (can be 0xFFFFFF for LBA)
0x08    4     LBA start (little-endian)
0x0C    4     Size in sectors (little-endian)
```

### 2. FAT32 Formatting

**FAT32 Boot Sector** (Sector 0 of partition, 512 bytes):
- **Offset 0x00-0x02** (3 bytes): Jump instruction (0xEB 0x?? 0x90)
- **Offset 0x03-0x0A** (8 bytes): OEM name ("MSWIN4.1")
- **Offset 0x0B-0x0C** (2 bytes): Bytes per sector (512)
- **Offset 0x0D** (1 byte): Sectors per cluster (8 for small drives)
- **Offset 0x0E-0x0F** (2 bytes): Reserved sectors (32)
- **Offset 0x10** (1 byte): Number of FATs (2)
- **Offset 0x11-0x12** (2 bytes): Root entries (0 for FAT32)
- **Offset 0x13-0x14** (2 bytes): Total sectors (0 for FAT32)
- **Offset 0x15** (1 byte): Media descriptor (0xF8)
- **Offset 0x16-0x17** (2 bytes): Sectors per FAT (0 for FAT32)
- **Offset 0x18-0x19** (2 bytes): Sectors per track (63)
- **Offset 0x1A-0x1B** (2 bytes): Number of heads (255)
- **Offset 0x1C-0x1F** (4 bytes): Hidden sectors (partition start LBA)
- **Offset 0x20-0x23** (4 bytes): Total sectors (partition size)
- **Offset 0x24-0x27** (4 bytes): Sectors per FAT (calculated)
- **Offset 0x28-0x29** (2 bytes): Flags (0x0000)
- **Offset 0x2A-0x2B** (2 bytes): Version (0x0000)
- **Offset 0x2C-0x2F** (4 bytes): Root cluster (2)
- **Offset 0x30-0x31** (2 bytes): FSInfo sector (1)
- **Offset 0x32-0x33** (2 bytes): Backup boot sector (6)
- **Offset 0x34-0x3F** (12 bytes): Reserved (zeros)
- **Offset 0x40** (1 byte): Drive number (0x80)
- **Offset 0x41** (1 byte): Reserved (0x00)
- **Offset 0x42** (1 byte): Extended boot signature (0x29)
- **Offset 0x43-0x46** (4 bytes): Volume serial number (random)
- **Offset 0x47-0x51** (11 bytes): Volume label ("NO NAME    ")
- **Offset 0x52-0x59** (8 bytes): Filesystem type ("FAT32   ")
- **Offset 0x5A-0x1FD** (420 bytes): Boot code (zeros)
- **Offset 0x1FE-0x1FF** (2 bytes): Boot signature (0x55AA)

### 3. FAT32 Calculations

**Sectors per FAT**:
```
FAT_size = ((total_sectors - reserved_sectors) / (sectors_per_cluster * 256 + 2)) + 1
```

**Cluster Count**:
```
data_sectors = total_sectors - reserved_sectors - (FAT_size * 2)
cluster_count = data_sectors / sectors_per_cluster
```

**FAT32 Requirements**:
- Cluster count must be >= 65,525 (otherwise it's FAT16)
- For small drives (<= 260 MB), use 1 sector per cluster
- For larger drives, use 8 sectors per cluster

---

## API Design

### New Functions

```c
/**
 * @brief Create a new MBR partition table with a single FAT32 partition
 * 
 * @param total_sectors Total number of sectors on the drive
 * @return esp_err_t ESP_OK on success
 */
esp_err_t usb_host_create_partition_table(uint32_t total_sectors);

/**
 * @brief Format a partition as FAT32
 * 
 * @param partition_num Partition number (0-3)
 * @param start_lba Starting LBA of the partition
 * @param size_sectors Size of the partition in sectors
 * @return esp_err_t ESP_OK on success
 */
esp_err_t usb_host_format_fat32(uint8_t partition_num, uint32_t start_lba, uint32_t size_sectors);

/**
 * @brief Get total number of sectors on the USB drive
 * 
 * @param total_sectors Pointer to store total sectors
 * @return esp_err_t ESP_OK on success
 */
esp_err_t usb_host_get_drive_capacity(uint32_t* total_sectors);
```

---

## Implementation Steps

### Step 1: Get Drive Capacity
- Use SCSI READ CAPACITY(10) command
- Extract total sectors from response
- Calculate drive size in MB

### Step 2: Create MBR Partition Table
- Zero out sector 0 (MBR)
- Create partition entry 1:
  - Boot indicator: 0x00 (non-bootable)
  - Partition type: 0x0C (FAT32 LBA)
  - Start LBA: 2048 (1 MB offset for alignment)
  - Size: total_sectors - 2048
- Set boot signature: 0x55AA
- Write MBR to sector 0

### Step 3: Format Partition as FAT32
- Calculate FAT32 parameters:
  - Sectors per cluster (1 or 8)
  - Sectors per FAT
  - Reserved sectors (32)
- Create FAT32 boot sector
- Write boot sector to partition start
- Create FSInfo sector
- Write FSInfo to sector 1 of partition
- Create backup boot sector
- Write backup to sector 6 of partition
- Initialize FAT tables (2 copies)
- Create root directory cluster

### Step 4: Mount New Partition
- Unmount existing VFS (if any)
- Re-install MSC device
- Mount VFS at /usb
- Verify mount successful

### Step 5: Verify Partition
- Create test file
- Write test data
- Read back and verify
- Delete test file

---

## Test Plan

### Test 1: Get Drive Capacity
- ✅ Read total sectors from USB drive
- ✅ Calculate drive size in MB
- ✅ Verify capacity matches expected value

### Test 2: Create MBR Partition Table
- ✅ Delete existing partitions
- ✅ Create new MBR with single FAT32 partition
- ✅ Verify MBR boot signature
- ✅ Verify partition entry

### Test 3: Format FAT32
- ✅ Format partition as FAT32
- ✅ Verify boot sector
- ✅ Verify FSInfo sector
- ✅ Verify FAT tables

### Test 4: Mount New Partition
- ✅ Mount newly created partition
- ✅ Verify mount successful
- ✅ List files (should be empty)

### Test 5: Write Test File
- ✅ Create test file
- ✅ Write test data
- ✅ Read back and verify
- ✅ Delete test file

---

## Safety Considerations

⚠️ **WARNING: DESTRUCTIVE OPERATIONS** ⚠️

This phase includes **DESTRUCTIVE OPERATIONS** that will:
- **DELETE ALL DATA** on the USB drive
- **ERASE ALL PARTITIONS**
- **FORMAT THE DRIVE**

**Safety Measures**:
1. ✅ Clear warning messages before operations
2. ✅ Require user confirmation (manual test)
3. ✅ Verify operations before writing
4. ✅ Detailed logging of all operations
5. ✅ Use test USB drive (not production data)

---

## Expected Results

### Before Phase 3b:
```
Partition Table: MBR
Partition Count: 1
Partition 0: Type=0x0E (FAT16 LBA), Start=2048, Size=94208 sectors (46 MB)
Files: 5 files, 2 directories
```

### After Phase 3b:
```
Partition Table: MBR
Partition Count: 1
Partition 0: Type=0x0C (FAT32 LBA), Start=2048, Size=<calculated> sectors
Files: 0 files, 0 directories (empty)
Test File: Created, written, verified, deleted
```

---

## Success Criteria

| Criterion | Target | Status |
|-----------|--------|--------|
| Get drive capacity | Success | ⏸️ |
| Create MBR partition table | Success | ⏸️ |
| Format FAT32 | Success | ⏸️ |
| Mount new partition | Success | ⏸️ |
| Write test file | Success | ⏸️ |
| Verify test file | Success | ⏸️ |
| No crashes | 0 crashes | ⏸️ |
| Code quality | Clean, documented | ⏸️ |

---

## Timeline

- **Planning**: 30 minutes ✅
- **Implementation**: 2-3 hours ⏸️
- **Testing**: 1 hour ⏸️
- **Documentation**: 30 minutes ⏸️
- **Total**: 4-5 hours

---

## Dependencies

- ✅ Phase 3a: Partition Detection & Deletion (COMPLETE)
- ✅ SCSI command support (READ(10), WRITE(10))
- ✅ Low-level sector access functions
- ⏸️ SCSI READ CAPACITY(10) command (to be implemented)

---

## Next Phase

**Phase 3c: File Copy from Internal Storage**
- Implement internal storage (SPIFFS/LittleFS)
- Store files in internal flash
- Copy files from internal storage to USB drive
- Verify file integrity

---

**Status**: 🚧 IN PROGRESS  
**Branch**: `feature/phase-3b-partition-creation`  
**Started**: November 8, 2025

