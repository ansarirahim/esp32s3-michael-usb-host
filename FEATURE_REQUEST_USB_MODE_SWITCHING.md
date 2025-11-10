# Feature Request: USB Mode Switching (Host ↔ Device)

**Date:** November 8, 2025  
**Requested By:** User  
**Analyzed By:** Abdul Raheem Ansari  
**Status:** FEASIBILITY ANALYSIS COMPLETE

---

## Executive Summary

✅ **TECHNICALLY FEASIBLE** - The ESP32-S3 USB OTG interface **CAN** switch between USB Host mode and USB Device mode at runtime through software.

⚠️ **IMPORTANT LIMITATION** - The two modes **CANNOT run simultaneously**. Only one mode can be active at a time.

✅ **RECOMMENDED APPROACH** - Create a new feature branch (`feature/usb-mode-switching`) to implement this functionality while keeping the current stable USB Host implementation intact.

---

## 1. Technical Feasibility Analysis

### 1.1 Official Espressif Documentation

**Source:** [ESP-FAQ - USB Peripherals](https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/peripherals/usb.html)

**Official Statement:**

> **Can the ESP32-S3's USB OTG interface be used in both USB Host and USB Device modes?**
>
> The ESP32-S3's USB OTG interface **can not be used as USB Host and USB Device at the same time**. However, **it is possible to switch between the USB Host mode and the USB Device mode by software**.
>
> If you need the standard negotiation function of USB OTG, please note that currently ESP32-S3 only supports this function on the hardware, and does not support it in software protocol.

**Conclusion:** ✅ **Runtime mode switching is officially supported and documented by Espressif.**

---

### 1.2 Hardware Capabilities

**ESP32-S3 USB OTG Peripheral:**
- **Controller:** USB DWC2 OTG controller
- **Speed:** USB 2.0 Full-Speed (12 Mbps)
- **Modes:** Host, Device, or OTG (hardware support only)
- **Pins:** GPIO 19 (D-), GPIO 20 (D+)
- **PHY:** Internal USB PHY

**Hardware Support:**
- ✅ USB Host mode (current implementation)
- ✅ USB Device mode (not yet implemented)
- ✅ Runtime mode switching (software-controlled)
- ❌ Simultaneous Host + Device operation (hardware limitation)
- ❌ OTG negotiation protocol (hardware support only, no software stack)

---

### 1.3 Software Stack Analysis

**Current Implementation:**
- **USB Host:** ESP-IDF USB Host library (`usb_host_msc`)
- **Stack:** Native ESP-IDF USB Host stack
- **Status:** Fully functional and stable

**Proposed USB Device Implementation:**
- **USB Device:** TinyUSB Device stack
- **Class:** MSC Device (Mass Storage Class)
- **Backend:** SPIFFS or FAT filesystem
- **Status:** Available in ESP-IDF examples

**TinyUSB Dual-Role Examples:**
- ✅ `examples/dual/host_info_to_device_cdc` - Demonstrates both Host and Device stacks
- ✅ Both stacks can be compiled into the same firmware
- ✅ Runtime switching is possible through re-initialization

---

## 2. Implementation Approach

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32-S3 Firmware                        │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐ │
│  │              Mode Manager (New Component)             │ │
│  │  - Current mode tracking                              │ │
│  │  - Mode switching logic                               │ │
│  │  - USB re-initialization                              │ │
│  │  - Button handler                                     │ │
│  │  - LED indication                                     │ │
│  └───────────────┬─────────────────────┬─────────────────┘ │
│                  │                     │                   │
│  ┌───────────────▼──────────┐  ┌──────▼──────────────────┐ │
│  │   USB Host Mode          │  │   USB Device Mode       │ │
│  │   (Current)              │  │   (New)                 │ │
│  │                          │  │                         │ │
│  │  - ESP-IDF USB Host      │  │  - TinyUSB Device       │ │
│  │  - MSC Host driver       │  │  - MSC Device class     │ │
│  │  - Format USB drives     │  │  - Expose SPIFFS        │ │
│  │  - Copy files to drives  │  │  - Read/write from PC   │ │
│  │  - Safe eject            │  │  - Appear as USB drive  │ │
│  └──────────────────────────┘  └─────────────────────────┘ │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐ │
│  │              Hardware Abstraction Layer               │ │
│  │  - USB PHY configuration                              │ │
│  │  - GPIO control (button, LED)                         │ │
│  │  - SPIFFS filesystem                                  │ │
│  └───────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

---

### 2.2 Mode Switching Workflow

```
┌─────────────────────────────────────────────────────────────┐
│                    Mode Switching Process                   │
└─────────────────────────────────────────────────────────────┘

Current Mode: USB HOST
         │
         │ [Button Press Detected]
         │
         ▼
    ┌────────────────────────────────────┐
    │ 1. Detect Button Press             │
    │    - Debounce (50ms)               │
    │    - Validate mode switch request  │
    └────────────┬───────────────────────┘
                 │
                 ▼
    ┌────────────────────────────────────┐
    │ 2. Deinitialize Current Mode       │
    │    - Stop workflow automation      │
    │    - Eject USB drive (if mounted)  │
    │    - Uninstall MSC Host driver     │
    │    - Close USB Host library        │
    │    - Deinitialize USB PHY          │
    └────────────┬───────────────────────┘
                 │
                 ▼
    ┌────────────────────────────────────┐
    │ 3. Reconfigure USB PHY             │
    │    - Set OTG mode to DEVICE        │
    │    - Reinitialize USB PHY          │
    │    - Configure USB pins            │
    └────────────┬───────────────────────┘
                 │
                 ▼
    ┌────────────────────────────────────┐
    │ 4. Initialize New Mode             │
    │    - Initialize TinyUSB Device     │
    │    - Configure MSC Device class    │
    │    - Mount SPIFFS as backend       │
    │    - Start USB Device stack        │
    └────────────┬───────────────────────┘
                 │
                 ▼
    ┌────────────────────────────────────┐
    │ 5. Update System State             │
    │    - Set current mode = DEVICE     │
    │    - Update LED color (Blue)       │
    │    - Log mode switch complete      │
    └────────────┬───────────────────────┘
                 │
                 ▼
         New Mode: USB DEVICE
```

**Switching Time:** Estimated 500-1000ms (includes USB re-enumeration)

---

### 2.3 Component Design

#### **2.3.1 Mode Manager Component**

**File:** `main/usb_mode_manager.c` / `main/usb_mode_manager.h`

**Responsibilities:**
- Track current USB mode (HOST or DEVICE)
- Handle button press events
- Coordinate mode switching sequence
- Update LED indication
- Provide mode query API

**Key Functions:**
```c
typedef enum {
    USB_MODE_HOST,      // USB Host mode (control flash drives)
    USB_MODE_DEVICE,    // USB Device mode (act as flash drive)
} usb_mode_t;

esp_err_t usb_mode_manager_init(gpio_num_t button_gpio);
esp_err_t usb_mode_switch(usb_mode_t new_mode);
usb_mode_t usb_mode_get_current(void);
bool usb_mode_is_switching(void);
```

---

#### **2.3.2 USB Device Mode Component**

**File:** `main/usb_device_msc.c` / `main/usb_device_msc.h`

**Responsibilities:**
- Initialize TinyUSB Device stack
- Configure MSC Device class
- Expose SPIFFS as mass storage
- Handle read/write operations from PC
- Manage USB device events

**Key Functions:**
```c
esp_err_t usb_device_msc_init(void);
esp_err_t usb_device_msc_deinit(void);
bool usb_device_msc_is_mounted(void);
```

**Backend Storage:**
- **Option 1:** SPIFFS (current internal storage)
- **Option 2:** FAT partition on internal flash
- **Recommended:** SPIFFS (already implemented and tested)

---

#### **2.3.3 Button Handler Component**

**File:** `main/button.c` / `main/button.h`

**Responsibilities:**
- GPIO interrupt handling
- Button debouncing (50ms)
- Long press detection (optional)
- Mode switch trigger

**Key Functions:**
```c
esp_err_t button_init(gpio_num_t gpio, button_callback_t callback);
void button_set_debounce_time(uint32_t ms);
```

---

### 2.4 LED Indication Scheme

**Current LED States (USB Host Mode):**
- 🟢 Green slow blink - IDLE (waiting for USB drive)
- 🔵 Cyan fast blink - PREPARE (USB drive detected)
- 🟣 Magenta blink - COPY (copying files)
- 🟢 Green solid 2s - SUCCESS (operation complete)
- 🔴 Red fast blink - ERROR (operation failed)

**New LED States (USB Device Mode):**
- 🔵 Blue slow blink - DEVICE_IDLE (waiting for PC connection)
- 🔵 Blue solid - DEVICE_MOUNTED (connected to PC)
- 🟣 Magenta blink - DEVICE_READ (PC reading data)
- 🟡 Yellow blink - DEVICE_WRITE (PC writing data)
- 🔴 Red fast blink - DEVICE_ERROR (error occurred)

**Mode Switching Indication:**
- 🟠 Orange fast blink (500ms) - MODE_SWITCHING

---

## 3. Use Cases

### 3.1 USB Host Mode (Current Functionality)

**Scenario:** Format and copy files to USB flash drives

**Workflow:**
1. ESP32-S3 powered on in USB Host mode
2. LED: Green slow blink (IDLE)
3. Insert USB flash drive into ESP32-S3 USB-OTG port
4. ESP32-S3 detects drive, formats with "MICHAEL-USB" label
5. ESP32-S3 copies 5 files from SPIFFS to drive
6. ESP32-S3 syncs and ejects drive
7. LED: Green solid 2s (SUCCESS)
8. Remove USB flash drive
9. Insert next drive → repeat

**Connection:**
```
Computer ──[USB]──→ ESP32-S3 Debug Port (power + serial)
                         ↓
                    USB-OTG Port ──[USB]──→ USB Flash Drive
```

---

### 3.2 USB Device Mode (New Functionality)

**Scenario:** ESP32-S3 appears as USB drive to computer

**Workflow:**
1. Press button to switch to USB Device mode
2. LED: Orange fast blink (switching)
3. Mode switch complete
4. LED: Blue slow blink (DEVICE_IDLE)
5. Connect ESP32-S3 USB-OTG port to computer
6. Computer detects ESP32-S3 as USB drive
7. LED: Blue solid (DEVICE_MOUNTED)
8. Computer can read/write files from/to SPIFFS
9. Computer ejects USB drive
10. LED: Blue slow blink (DEVICE_IDLE)
11. Disconnect USB cable

**Connection:**
```
Computer ──[USB]──→ ESP32-S3 USB-OTG Port
                         ↑
                    (Acts as USB flash drive)
```

---

### 3.3 Mode Switching Use Case

**Scenario:** Switch between Host and Device modes

**Workflow:**
1. **Current mode:** USB Host (formatting flash drives)
2. **User action:** Press mode switch button
3. **System response:**
   - LED: Orange fast blink (switching)
   - Deinitialize USB Host
   - Reconfigure USB PHY
   - Initialize USB Device
   - LED: Blue slow blink (DEVICE_IDLE)
4. **New mode:** USB Device (ready to connect to PC)
5. **User action:** Connect to PC
6. **System response:**
   - LED: Blue solid (DEVICE_MOUNTED)
   - PC can access SPIFFS files
7. **User action:** Press mode switch button again
8. **System response:**
   - LED: Orange fast blink (switching)
   - Deinitialize USB Device
   - Reconfigure USB PHY
   - Initialize USB Host
   - LED: Green slow blink (IDLE)
9. **New mode:** USB Host (ready for flash drives)

---

## 4. Git Workflow Recommendation

### 4.1 Branching Strategy

✅ **RECOMMENDED:** Create a new feature branch

**Branch Name:** `feature/usb-mode-switching`

**Rationale:**
- Keeps current stable USB Host implementation intact
- Allows independent development and testing
- Enables easy rollback if issues arise
- Follows Git Flow best practices
- Facilitates code review and testing

**Branch Structure:**
```
master (production)
  │
  ├── develop (integration)
  │     │
  │     ├── feature/phase-3c-file-copy (current - USB Host complete)
  │     │
  │     └── feature/usb-mode-switching (new - add mode switching)
  │           │
  │           ├── Implement USB Device mode
  │           ├── Implement Mode Manager
  │           ├── Implement Button Handler
  │           ├── Update LED control
  │           ├── Integration testing
  │           └── Merge to develop when stable
  │
  └── (merge develop to master for release)
```

---

### 4.2 Development Phases

**Phase 1: USB Device Mode Implementation**
- Implement TinyUSB Device stack integration
- Implement MSC Device class
- Implement SPIFFS backend
- Test USB Device mode standalone
- **Deliverable:** USB Device mode working independently

**Phase 2: Mode Manager Implementation**
- Implement mode tracking
- Implement USB re-initialization logic
- Implement mode switching sequence
- Test mode switching without button
- **Deliverable:** Programmatic mode switching working

**Phase 3: Button Handler Implementation**
- Implement GPIO button handler
- Implement debouncing
- Integrate with Mode Manager
- Test button-triggered mode switching
- **Deliverable:** Button-triggered mode switching working

**Phase 4: LED Indication Update**
- Update LED states for Device mode
- Implement mode switching indication
- Test all LED states
- **Deliverable:** Complete LED indication system

**Phase 5: Integration and Testing**
- Integration testing of all components
- Performance testing
- Stability testing (multiple mode switches)
- Documentation
- **Deliverable:** Complete feature ready for merge

---

## 5. Implementation Complexity

### 5.1 Estimated Development Effort

| Phase | Component | Complexity | Estimated Time |
|-------|-----------|------------|----------------|
| 1 | USB Device MSC | Medium | 8-12 hours |
| 2 | Mode Manager | Medium | 6-8 hours |
| 3 | Button Handler | Low | 2-4 hours |
| 4 | LED Updates | Low | 2-3 hours |
| 5 | Integration & Testing | Medium | 6-8 hours |
| **Total** | | **Medium** | **24-35 hours** |

**Complexity Rating:** ⭐⭐⭐ Medium (3/5)

**Rationale:**
- USB Device mode: Well-documented in ESP-IDF examples
- TinyUSB: Mature and stable library
- Mode switching: Straightforward re-initialization
- Button handling: Standard GPIO interrupt
- LED control: Already implemented

---

### 5.2 Technical Challenges

**Challenge 1: USB Re-initialization**
- **Issue:** Proper cleanup of USB resources before mode switch
- **Solution:** Follow ESP-IDF USB deinitialization sequence
- **Risk:** Low (well-documented)

**Challenge 2: SPIFFS Access Conflicts**
- **Issue:** SPIFFS accessed by both Host and Device modes
- **Solution:** Ensure SPIFFS is unmounted before mode switch
- **Risk:** Low (standard filesystem management)

**Challenge 3: USB Enumeration Timing**
- **Issue:** PC may not immediately recognize USB device after switch
- **Solution:** Add delay after mode switch, implement USB connect/disconnect
- **Risk:** Low (standard USB behavior)

**Challenge 4: Button Debouncing**
- **Issue:** False triggers due to button bounce
- **Solution:** Implement 50ms debounce timer
- **Risk:** Very Low (standard technique)

**Challenge 5: Mode Switch During Active Operation**
- **Issue:** User presses button while copying files
- **Solution:** Disable mode switch during active operations
- **Risk:** Low (state machine logic)

---

## 6. Hardware Requirements

### 6.1 Additional Hardware

**Required:**
- ✅ Push button (already available on most dev boards)
- ✅ Pull-up resistor (10kΩ) - usually built-in
- ✅ USB-OTG connector (already present)

**Optional:**
- ⭕ External button if dev board doesn't have one
- ⭕ LED indicator (already implemented - WS2812B RGB)

**GPIO Allocation:**
- **Button GPIO:** GPIO 0 (BOOT button) or GPIO 1-10 (configurable)
- **LED GPIO:** GPIO 48 (already configured)
- **USB OTG:** GPIO 19, 20 (fixed)

---

### 6.2 Hardware Limitations

**Limitation 1: Single USB OTG Port**
- **Impact:** Cannot use Host and Device modes simultaneously
- **Workaround:** Mode switching (as proposed)

**Limitation 2: USB Speed**
- **Impact:** Full-Speed only (12 Mbps), not High-Speed (480 Mbps)
- **Workaround:** None (hardware limitation)

**Limitation 3: SPIFFS Size**
- **Impact:** Limited storage capacity for USB Device mode
- **Workaround:** Use FAT partition or external flash (future enhancement)

---

## 7. Potential Risks and Limitations

### 7.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| USB re-initialization fails | Low | High | Implement retry logic, watchdog timer |
| Mode switch during file transfer | Medium | Medium | Disable button during active operations |
| SPIFFS corruption | Low | High | Implement proper unmount sequence |
| PC doesn't recognize device | Low | Medium | Implement USB connect/disconnect |
| Memory fragmentation | Low | Low | Monitor heap usage, implement cleanup |

---

### 7.2 Functional Limitations

**Limitation 1: No Simultaneous Operation**
- **Description:** Cannot use Host and Device modes at the same time
- **Impact:** User must choose one mode at a time
- **Acceptable:** Yes (per Espressif documentation)

**Limitation 2: Mode Switch Delay**
- **Description:** 500-1000ms delay during mode switching
- **Impact:** Brief interruption in operation
- **Acceptable:** Yes (one-time delay)

**Limitation 3: No OTG Negotiation**
- **Description:** No automatic mode detection (OTG protocol)
- **Impact:** User must manually switch modes
- **Acceptable:** Yes (button-triggered switching)

**Limitation 4: SPIFFS Read-Only in Device Mode**
- **Description:** PC can read but not write to SPIFFS (optional limitation)
- **Impact:** Depends on use case
- **Acceptable:** Configurable (can enable write if needed)

---

## 8. Testing Strategy

### 8.1 Unit Testing

**Test 1: USB Device Mode Initialization**
- Initialize USB Device mode
- Verify TinyUSB stack starts
- Verify MSC class registers
- Verify SPIFFS mounts as backend

**Test 2: USB Device Mode Operation**
- Connect to PC
- Verify PC detects USB drive
- Read files from SPIFFS via PC
- Write files to SPIFFS via PC (if enabled)
- Eject from PC

**Test 3: Mode Switching**
- Switch from Host to Device
- Verify USB Host deinitializes
- Verify USB Device initializes
- Switch from Device to Host
- Verify USB Device deinitializes
- Verify USB Host initializes

**Test 4: Button Handler**
- Press button
- Verify debouncing works
- Verify mode switch triggers
- Verify LED indication updates

---

### 8.2 Integration Testing

**Test 1: Complete Workflow - Host Mode**
- Power on in Host mode
- Insert USB drive
- Verify format and copy
- Eject USB drive
- Verify success

**Test 2: Complete Workflow - Device Mode**
- Switch to Device mode
- Connect to PC
- Verify PC access to files
- Disconnect from PC
- Verify cleanup

**Test 3: Mode Switching Cycle**
- Start in Host mode
- Process 1 USB drive
- Switch to Device mode
- Connect to PC
- Disconnect from PC
- Switch to Host mode
- Process another USB drive
- Verify no errors

**Test 4: Stress Testing**
- Perform 100 mode switches
- Verify no memory leaks
- Verify no crashes
- Verify consistent behavior

---

## 9. Documentation Requirements

### 9.1 User Documentation

**Document 1: User Guide**
- How to switch between modes
- LED indication reference
- Troubleshooting guide

**Document 2: Hardware Setup**
- Button connection diagram
- USB connection diagrams for each mode
- GPIO configuration

**Document 3: Configuration Guide**
- How to enable/disable write access in Device mode
- How to change button GPIO
- How to customize LED colors

---

### 9.2 Technical Documentation

**Document 1: Architecture Design**
- Component diagram
- Sequence diagrams
- State machine diagrams

**Document 2: API Reference**
- Mode Manager API
- USB Device API
- Button Handler API

**Document 3: Testing Report**
- Test cases and results
- Performance metrics
- Known issues and limitations

---

## 10. Recommendation

### 10.1 Feasibility Verdict

✅ **RECOMMENDED FOR IMPLEMENTATION**

**Justification:**
1. ✅ Officially supported by Espressif
2. ✅ Well-documented in ESP-IDF
3. ✅ Medium complexity (manageable)
4. ✅ Clear use cases
5. ✅ No major technical blockers
6. ✅ Enhances product value

---

### 10.2 Implementation Plan

**Step 1: Create Feature Branch**
```bash
git checkout develop
git pull origin develop
git checkout -b feature/usb-mode-switching
git push -u origin feature/usb-mode-switching
```

**Step 2: Implement in Phases**
- Phase 1: USB Device Mode (8-12 hours)
- Phase 2: Mode Manager (6-8 hours)
- Phase 3: Button Handler (2-4 hours)
- Phase 4: LED Updates (2-3 hours)
- Phase 5: Integration & Testing (6-8 hours)

**Step 3: Testing and Validation**
- Unit testing
- Integration testing
- Stress testing
- Documentation

**Step 4: Merge to Develop**
```bash
git checkout develop
git merge feature/usb-mode-switching
git push origin develop
```

**Step 5: Production Release**
```bash
git checkout master
git merge develop
git push origin master
git tag -a v2.0.0 -m "USB Mode Switching Feature"
git push origin v2.0.0
```

---

### 10.3 Timeline Estimate

**Total Development Time:** 24-35 hours  
**Calendar Time:** 1-2 weeks (part-time development)

**Milestones:**
- Week 1: Phases 1-3 (USB Device + Mode Manager + Button)
- Week 2: Phases 4-5 (LED Updates + Integration + Testing)

---

## 11. Conclusion

The USB mode switching feature is **technically feasible** and **recommended for implementation**. The ESP32-S3 officially supports runtime switching between USB Host and USB Device modes, and the implementation complexity is manageable.

**Key Takeaways:**
- ✅ Officially supported by Espressif
- ✅ Medium complexity (24-35 hours)
- ✅ Clear use cases and benefits
- ✅ No major technical risks
- ✅ Recommended to use separate feature branch
- ⚠️ Cannot run both modes simultaneously (hardware limitation)
- ⚠️ 500-1000ms delay during mode switching (acceptable)

**Next Steps:**
1. Approve feature request
2. Create feature branch
3. Begin Phase 1 implementation
4. Iterative development and testing
5. Merge to develop when stable

---

**Document prepared for:** User  
**Date:** November 8, 2025  
**Status:** FEASIBILITY ANALYSIS COMPLETE  
**Recommendation:** ✅ PROCEED WITH IMPLEMENTATION

