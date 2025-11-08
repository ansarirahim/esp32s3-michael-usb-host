# Phase 3a: Partition Detection & Deletion - Implementation Plan

## Overview

**Phase 3a** implements partition detection and deletion functionality, enabling the ESP32-S3 to:
- Read raw sectors from USB drives using SCSI commands
- Detect partition table type (MBR or GPT)
- Read partition information (count, type, size)
- Delete all partitions by zeroing the MBR

**Status**: 🚧 **IN PROGRESS**  
**Branch**: `feature/phase-3a-partition-detection`  
**Date**: November 8, 2025

---

## Technical Background

### MBR (Master Boot Record) Structure

The MBR is located at **sector 0** (LBA 0) of the disk and is **512 bytes** in size.

#### MBR Layout:
```
Offset  Size  Description
------  ----  -----------
0x000   446   Bootstrap code
0x1BE    16   Partition entry 1
0x1CE    16   Partition entry 2
0x1DE    16   Partition entry 3
0x1EE    16   Partition entry 4
0x1FE     2   Boot signature (0x55 0xAA)
```

#### Partition Entry Structure (16 bytes):
```
Offset  Size  Description
------  ----  -----------
0x00      1   Boot indicator (0x80 = bootable, 0x00 = non-bootable)
0x01      3   Starting CHS address (obsolete)
0x04      1   Partition type
0x05      3   Ending CHS address (obsolete)
0x08      4   Starting LBA (little-endian)
0x0C      4   Size in sectors (little-endian)
```

#### Common Partition Types:
- `0x00` - Empty/unused
- `0x01` - FAT12
- `0x04` - FAT16 (< 32 MB)
- `0x06` - FAT16 (>= 32 MB)
- `0x0B` - FAT32 (CHS)
- `0x0C` - FAT32 (LBA)
- `0x0E` - FAT16 (LBA)
- `0x0F` - Extended partition
- `0x83` - Linux
- `0xEE` - GPT protective MBR

### GPT (GUID Partition Table) Structure

GPT uses a protective MBR at sector 0 with partition type `0xEE`, followed by:
- **Sector 1**: GPT header
- **Sectors 2-33**: Partition entries (128 entries, 128 bytes each)

For Phase 3a, we'll focus on **MBR detection and deletion**. GPT support can be added later if needed.

---

## Implementation Plan

### 1. Low-Level Sector Access Functions

#### Function: `usb_host_read_sector()`
**Purpose**: Read raw sector from USB drive  
**Location**: `main/usb_host.c`

```c
/**
 * @brief Read raw sector from USB drive
 * @param sector_num Sector number (LBA)
 * @param buffer Buffer to store sector data (must be at least 512 bytes)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_read_sector(uint32_t sector_num, uint8_t* buffer);
```

**Implementation**:
- Use `msc_host_read_sector()` from MSC driver
- Read 512-byte sectors
- Validate buffer size
- Error handling

#### Function: `usb_host_write_sector()`
**Purpose**: Write raw sector to USB drive  
**Location**: `main/usb_host.c`

```c
/**
 * @brief Write raw sector to USB drive
 * @param sector_num Sector number (LBA)
 * @param buffer Buffer containing sector data (must be 512 bytes)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_write_sector(uint32_t sector_num, const uint8_t* buffer);
```

**Implementation**:
- Use `msc_host_write_sector()` from MSC driver
- Write 512-byte sectors
- Validate buffer size
- Error handling

---

### 2. Partition Detection Functions

#### Function: `usb_host_detect_partition_table()`
**Purpose**: Detect partition table type (MBR or GPT)  
**Location**: `main/usb_host.c`

```c
typedef enum {
    PARTITION_TABLE_NONE = 0,
    PARTITION_TABLE_MBR,
    PARTITION_TABLE_GPT,
    PARTITION_TABLE_UNKNOWN
} partition_table_type_t;

/**
 * @brief Detect partition table type
 * @param table_type Pointer to store partition table type
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_detect_partition_table(partition_table_type_t* table_type);
```

**Implementation**:
1. Read sector 0 (MBR)
2. Check boot signature (0x55AA at offset 0x1FE)
3. Check partition entry 1 type:
   - If `0xEE` → GPT
   - If `0x00` → No partitions
   - Otherwise → MBR
4. Return partition table type

#### Function: `usb_host_get_partition_count()`
**Purpose**: Get number of partitions  
**Location**: `main/usb_host.c`

```c
/**
 * @brief Get number of partitions on USB drive
 * @param count Pointer to store partition count
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_get_partition_count(uint8_t* count);
```

**Implementation**:
1. Read MBR (sector 0)
2. Check each of 4 partition entries
3. Count entries with non-zero type
4. Return count

#### Structure: `partition_info_t`
**Purpose**: Store partition information

```c
typedef struct {
    uint8_t boot_indicator;     /**< 0x80 = bootable, 0x00 = non-bootable */
    uint8_t partition_type;     /**< Partition type code */
    uint32_t start_lba;         /**< Starting LBA */
    uint32_t size_sectors;      /**< Size in sectors */
    uint64_t size_bytes;        /**< Size in bytes */
} partition_info_t;
```

#### Function: `usb_host_get_partition_info()`
**Purpose**: Get information about a specific partition  
**Location**: `main/usb_host.c`

```c
/**
 * @brief Get partition information
 * @param partition_num Partition number (0-3)
 * @param info Pointer to store partition information
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_get_partition_info(uint8_t partition_num, partition_info_t* info);
```

**Implementation**:
1. Validate partition number (0-3)
2. Read MBR (sector 0)
3. Parse partition entry at offset `0x1BE + (partition_num * 16)`
4. Extract partition information
5. Calculate size in bytes
6. Return partition info

---

### 3. Partition Deletion Functions

#### Function: `usb_host_delete_all_partitions()`
**Purpose**: Delete all partitions by zeroing MBR  
**Location**: `main/usb_host.c`

```c
/**
 * @brief Delete all partitions (zero MBR)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t usb_host_delete_all_partitions(void);
```

**Implementation**:
1. **Safety check**: Ensure USB drive is mounted
2. **Unmount VFS**: Must unmount before raw sector access
3. **Create zero buffer**: 512 bytes of zeros
4. **Write to sector 0**: Zero out MBR
5. **Verification**: Read back sector 0 to confirm
6. **Re-mount**: Mount VFS again (will fail - no partitions)
7. Return success

**Safety Considerations**:
- ⚠️ **DESTRUCTIVE OPERATION** - All data will be lost!
- Must unmount VFS before writing to sector 0
- Should add confirmation mechanism
- Should log operation clearly

---

## API Design

### Header File Updates (`main/usb_host.h`)

Add new function declarations:

```c
/* Partition table types */
typedef enum {
    PARTITION_TABLE_NONE = 0,
    PARTITION_TABLE_MBR,
    PARTITION_TABLE_GPT,
    PARTITION_TABLE_UNKNOWN
} partition_table_type_t;

/* Partition information */
typedef struct {
    uint8_t boot_indicator;
    uint8_t partition_type;
    uint32_t start_lba;
    uint32_t size_sectors;
    uint64_t size_bytes;
} partition_info_t;

/* Low-level sector access */
esp_err_t usb_host_read_sector(uint32_t sector_num, uint8_t* buffer);
esp_err_t usb_host_write_sector(uint32_t sector_num, const uint8_t* buffer);

/* Partition detection */
esp_err_t usb_host_detect_partition_table(partition_table_type_t* table_type);
esp_err_t usb_host_get_partition_count(uint8_t* count);
esp_err_t usb_host_get_partition_info(uint8_t partition_num, partition_info_t* info);

/* Partition deletion */
esp_err_t usb_host_delete_all_partitions(void);
```

---

## Testing Plan

### Test Sequence

1. **Test 1: Sector Read**
   - Read sector 0 (MBR)
   - Verify 512 bytes read
   - Check boot signature (0x55AA)

2. **Test 2: Partition Detection**
   - Detect partition table type
   - Get partition count
   - Display partition information

3. **Test 3: Partition Deletion**
   - Delete all partitions
   - Verify MBR is zeroed
   - Confirm no partitions detected

4. **Test 4: Re-detection**
   - Re-detect partition table
   - Verify no partitions found
   - Confirm ready for formatting

### Expected Results

**Before Deletion**:
```
Partition Table: MBR
Partition Count: 1
Partition 0:
  Type: 0x0C (FAT32 LBA)
  Start LBA: 2048
  Size: 15,138,816 sectors (7.75 GB)
```

**After Deletion**:
```
Partition Table: NONE
Partition Count: 0
MBR zeroed successfully
Ready for formatting
```

---

## Implementation Steps

1. ✅ Create implementation plan (this document)
2. ⏭️ Create new branch `feature/phase-3a-partition-detection`
3. ⏭️ Implement low-level sector access functions
4. ⏭️ Implement partition detection functions
5. ⏭️ Implement partition deletion function
6. ⏭️ Add automated tests to `main/main.c`
7. ⏭️ Test with real USB drive
8. ⏭️ Document results
9. ⏭️ Commit and push changes

---

## Safety Considerations

### Partition Deletion Warnings

⚠️ **CRITICAL**: Partition deletion is a **DESTRUCTIVE OPERATION**!

**Safety Measures**:
1. **Unmount VFS first** - Prevent file system corruption
2. **Clear logging** - Log every step of deletion process
3. **Verification** - Read back MBR to confirm deletion
4. **LED feedback** - Use ERROR state during deletion
5. **No auto-delete** - Require explicit user action

**Recommended Flow**:
```
1. Detect partitions
2. Display partition info
3. Wait for user confirmation
4. Unmount VFS
5. Delete partitions (zero MBR)
6. Verify deletion
7. Ready for Phase 3b (formatting)
```

---

## Next Phase

After Phase 3a is complete, proceed to:

**Phase 3b: Partition Creation & Formatting**
- Create new MBR partition table
- Create single FAT32 partition
- Format partition as FAT32
- Mount new partition

---

**Created By**: Abdul Raheem Ansari  
**Date**: November 8, 2025  
**Status**: 🚧 **IN PROGRESS**

