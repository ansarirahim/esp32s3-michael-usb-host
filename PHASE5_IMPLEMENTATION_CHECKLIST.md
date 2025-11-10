# Phase 5: USB Mode Switching - Implementation Checklist

**Document Version:** 1.0
**Date:** November 8, 2025
**Author:** Augment Agent (AI Engineering Assistant)
**Project:** ESP32-S3 USB Host Automator for Michael Steinmann
**Repository:** https://github.com/ansarirahim/esp32s3-michael-usb-host
**Feature:** Runtime Triple-Press Button Toggle with Serial Command Fallback
**Estimated Effort:** 36 hours (1 week part-time)
**Branch:** `feature/usb-mode-switching`

---

## Pre-Implementation

- [ ] **Review recommendation document** (PHASE5_USB_MODE_SWITCHING_RECOMMENDATION.md)
- [ ] **Approve implementation approach** with stakeholders
- [ ] **Create feature branch**
  ```bash
  git checkout develop
  git pull origin develop
  git checkout -b feature/usb-mode-switching
  git push -u origin feature/usb-mode-switching
  ```
- [ ] **Set up development environment** (ESP-IDF v5.5.1, toolchain)
- [ ] **Backup current stable implementation** (feature/phase-3c-file-copy)

---

## Phase 1: Core Mode Switching (10 hours)

### Task 1.1: Button Handler Implementation (4 hours)

**File:** `main/button.c` / `main/button.h`

- [ ] **Create button.h header file**
  - [ ] Define `button_callback_t` function pointer type
  - [ ] Declare `button_init()` function
  - [ ] Declare `button_set_triple_press_timeout()` function
  - [ ] Add Doxygen documentation

- [ ] **Create button.c implementation file**
  - [ ] Implement GPIO interrupt handler (falling edge on GPIO 0)
  - [ ] Implement 50ms debounce timer (FreeRTOS timer)
  - [ ] Implement triple-press detection logic
    - [ ] Track press count (0-3)
    - [ ] Start 2-second timeout timer on first press
    - [ ] Increment press count on each valid press
    - [ ] Call callback on third press
    - [ ] Reset press count on timeout
  - [ ] Add error handling (excessive presses, GPIO init failure)
  - [ ] Add logging (ESP_LOGI for each press, ESP_LOGD for debug)

- [ ] **Test button handler standalone**
  - [ ] Single press → No callback (press count resets after 2s)
  - [ ] Double press → No callback (press count resets after 2s)
  - [ ] Triple press (within 2s) → Callback triggered ✅
  - [ ] Triple press (over 3s) → No callback (timeout between presses)
  - [ ] Rapid presses (100x) → No false triggers (debouncing works)

**Deliverable:** Button handler working with triple-press detection

---

### Task 1.2: Serial Command Implementation (2 hours)

**File:** `main/console_commands.c` (new) or add to existing console

- [ ] **Create console_commands.c file** (if not exists)
  - [ ] Include ESP console component headers
  - [ ] Implement `cmd_mode_switch()` function
    - [ ] Parse arguments: `host`, `device`, `toggle`, `status`
    - [ ] Call `usb_mode_manager_switch()` with appropriate mode
    - [ ] Print confirmation message
    - [ ] Handle errors (invalid argument, switch failed)
  - [ ] Register commands in `console_commands_register()`
    - [ ] `mode host` - Switch to USB Host mode
    - [ ] `mode device` - Switch to USB Device mode
    - [ ] `mode toggle` - Toggle between modes
    - [ ] `mode status` - Show current mode and statistics

- [ ] **Integrate with main.c**
  - [ ] Call `console_commands_register()` in `app_main()`
  - [ ] Ensure console is initialized before registering commands

- [ ] **Test serial commands**
  - [ ] `mode host` → Switch to Host mode ✅
  - [ ] `mode device` → Switch to Device mode ✅
  - [ ] `mode toggle` → Toggle mode ✅
  - [ ] `mode status` → Display current mode ✅
  - [ ] `mode invalid` → Error message ✅

**Deliverable:** Serial commands working for mode switching

---

### Task 1.3: Mode Switching Integration (1 hour)

**File:** `main/usb_mode_manager.c` (already designed in FEATURE_REQUEST doc)

- [ ] **Verify mode manager implementation**
  - [ ] `usb_mode_manager_init()` - Initialize mode manager
  - [ ] `usb_mode_manager_switch()` - Switch between modes
  - [ ] `usb_mode_manager_get_current_mode()` - Get current mode
  - [ ] Safety checks (USB active, mode already set)
  - [ ] Deinitialize current USB mode
  - [ ] Reconfigure USB PHY
  - [ ] Initialize new USB mode
  - [ ] Update LED indication

- [ ] **Integrate button callback**
  - [ ] In `button_init()`, pass `usb_mode_manager_toggle()` as callback
  - [ ] Ensure callback is called from button handler on triple-press

- [ ] **Integrate serial command**
  - [ ] In `cmd_mode_switch()`, call `usb_mode_manager_switch()`
  - [ ] Handle return value (success, error)

**Deliverable:** Mode switching working via both button and serial command

---

### Task 1.4: LED Indication Update (1 hour)

**File:** `main/led_control.c` (existing)

- [ ] **Add new LED states**
  - [ ] `LED_STATE_MODE_SWITCHING` - Orange fast blink (200ms)
  - [ ] `LED_STATE_DEVICE_IDLE` - Blue slow blink (1000ms)
  - [ ] `LED_STATE_DEVICE_MOUNTED` - Blue solid
  - [ ] `LED_STATE_DEVICE_ACTIVE` - Magenta blink (500ms)

- [ ] **Update LED state machine**
  - [ ] Add transitions for new states
  - [ ] Ensure LED updates during mode switching

- [ ] **Test LED indication**
  - [ ] Mode switching → Orange fast blink ✅
  - [ ] Host mode idle → Green slow blink ✅
  - [ ] Device mode idle → Blue slow blink ✅
  - [ ] Error → Red fast blink ✅

**Deliverable:** Complete LED indication for all modes and states

---

### Task 1.5: Unit Testing (2 hours)

- [ ] **Test button handler**
  - [ ] 100+ button presses (no false triggers)
  - [ ] Triple-press detection (10 successful switches)
  - [ ] Timeout behavior (press count resets after 2s)
  - [ ] Debouncing (no bounce-induced false triggers)

- [ ] **Test serial commands**
  - [ ] All commands work correctly
  - [ ] Error handling (invalid arguments)
  - [ ] Confirmation messages displayed

- [ ] **Test mode switching**
  - [ ] Host → Device → Host (10 cycles)
  - [ ] LED indication correct for each mode
  - [ ] Serial output correct for each mode

**Deliverable:** Phase 1 complete and tested

---

## Phase 2: Enhanced Features (11 hours)

### Task 2.1: Safety Mechanisms (2 hours)

**File:** `main/usb_mode_manager.c`

- [ ] **Implement safety checks**
  - [ ] Check if USB drive is mounted (Host mode)
    - [ ] Call `usb_host_is_mounted()`
    - [ ] If mounted → Refuse mode switch, return error
  - [ ] Check if PC is connected (Device mode)
    - [ ] Call `usb_device_is_connected()`
    - [ ] If connected → Refuse mode switch, return error
  - [ ] Check if file transfer is in progress
    - [ ] Call `workflow_is_active()`
    - [ ] If active → Refuse mode switch, return error

- [ ] **Implement error feedback**
  - [ ] LED: Red fast blink (2 seconds)
  - [ ] Serial message: "Cannot switch mode: USB operation in progress"
  - [ ] Return to previous LED state

- [ ] **Test safety mechanisms**
  - [ ] Attempt switch during file copy → Refused ✅
  - [ ] Attempt switch with USB drive mounted → Refused ✅
  - [ ] Attempt switch with PC connected → Refused ✅
  - [ ] Switch after operation complete → Success ✅

**Deliverable:** Safety mechanisms prevent accidental switches during USB operations

---

### Task 2.2: Fail-Safe Behavior (2 hours)

**File:** `main/usb_mode_manager.c`

- [ ] **Implement retry logic**
  - [ ] If USB initialization fails → Retry 3 times (1-second delay)
  - [ ] Log each retry attempt
  - [ ] If all retries fail → Revert to USB Host mode (safe default)

- [ ] **Implement fail-safe mode**
  - [ ] On critical error → Revert to USB Host mode
  - [ ] LED: Red fast blink → Green (Host mode restored)
  - [ ] Serial message: "Mode switch failed, reverted to Host mode"

- [ ] **Test fail-safe behavior**
  - [ ] Simulate USB init failure → Retries 3 times → Reverts to Host ✅
  - [ ] Verify LED indication correct
  - [ ] Verify serial messages correct

**Deliverable:** Fail-safe behavior ensures system always recovers to Host mode

---

### Task 2.3: Watchdog Timer (1 hour)

**File:** `main/usb_mode_manager.c`

- [ ] **Implement watchdog timer**
  - [ ] Initialize task watchdog (10-second timeout)
  - [ ] Add current task to watchdog
  - [ ] Reset watchdog during mode switch
  - [ ] If timeout → System resets automatically

- [ ] **Test watchdog timer**
  - [ ] Normal mode switch → Watchdog reset, no timeout ✅
  - [ ] Simulate hang (infinite loop) → Watchdog triggers reset ✅
  - [ ] Verify system boots into Host mode after reset ✅

**Deliverable:** Watchdog timer prevents system hang during mode switch

---

### Task 2.4: Complete LED States (1 hour)

**File:** `main/led_control.c`

- [ ] **Implement all LED states** (see section 7.1 in recommendation doc)
  - [ ] Boot timeout (if using boot-time approach)
  - [ ] Safe mode (if implementing safe mode)
  - [ ] All Host mode states
  - [ ] All Device mode states
  - [ ] All error states

- [ ] **Test all LED states**
  - [ ] Verify each state displays correct color and pattern
  - [ ] Verify transitions between states are smooth

**Deliverable:** Complete LED indication scheme implemented

---

### Task 2.5: Error Handling and Logging (2 hours)

**File:** `main/usb_mode_manager.c`, `main/button.c`, `main/console_commands.c`

- [ ] **Implement comprehensive error handling**
  - [ ] All functions return `esp_err_t`
  - [ ] All errors logged with ESP_LOGE
  - [ ] All warnings logged with ESP_LOGW
  - [ ] All info messages logged with ESP_LOGI

- [ ] **Implement error messages** (see section 7.2 in recommendation doc)
  - [ ] Mode switching messages
  - [ ] Button press messages
  - [ ] Serial command messages
  - [ ] Error messages
  - [ ] Safety messages

- [ ] **Test error handling**
  - [ ] Trigger each error scenario
  - [ ] Verify correct error message displayed
  - [ ] Verify system recovers gracefully

**Deliverable:** Comprehensive error handling and logging

---

### Task 2.6: Integration Testing (3 hours)

- [ ] **Test mode switching (Host → Device)**
  - [ ] Start in Host mode (green LED)
  - [ ] Triple-press BOOT button
  - [ ] LED blinks orange (switching)
  - [ ] LED changes to blue (Device mode)
  - [ ] Connect to PC → ESP32 appears as USB drive ✅

- [ ] **Test mode switching (Device → Host)**
  - [ ] Start in Device mode (blue LED)
  - [ ] Triple-press BOOT button
  - [ ] LED blinks orange (switching)
  - [ ] LED changes to green (Host mode)
  - [ ] Insert USB drive → Workflow executes ✅

- [ ] **Test safety mechanisms**
  - [ ] Attempt mode switch during file copy → Refused ✅
  - [ ] Attempt mode switch during USB mount → Refused ✅
  - [ ] Mode switch after operation complete → Success ✅

- [ ] **Test fail-safe behavior**
  - [ ] Simulate USB init failure → Reverts to Host ✅
  - [ ] Simulate watchdog timeout → System resets → Boots into Host ✅

**Deliverable:** Phase 2 complete and tested

---

## Phase 3: Testing and Validation (15 hours)

### Task 3.1: Unit Testing (2 hours)

- [ ] **Button handler tests**
  - [ ] 100+ button presses (no false triggers)
  - [ ] Triple-press detection (10 successful switches)
  - [ ] Timeout behavior (press count resets)
  - [ ] Debouncing (no bounce-induced false triggers)

- [ ] **Serial command tests**
  - [ ] All commands work correctly
  - [ ] Error handling (invalid arguments)
  - [ ] Confirmation messages displayed

- [ ] **Mode switching tests**
  - [ ] Host → Device → Host (10 cycles)
  - [ ] LED indication correct
  - [ ] Serial output correct

**Deliverable:** All unit tests passing

---

### Task 3.2: Integration Testing (3 hours)

- [ ] **Full workflow tests**
  - [ ] Host mode: Insert USB drive → Format → Copy → Eject ✅
  - [ ] Device mode: Connect to PC → PC accesses SPIFFS ✅
  - [ ] Mode switch: Host → Device → Host (with USB operations) ✅

- [ ] **Safety mechanism tests**
  - [ ] Mode switch during file copy → Refused ✅
  - [ ] Mode switch during USB mount → Refused ✅
  - [ ] Mode switch after operation complete → Success ✅

- [ ] **Error recovery tests**
  - [ ] USB init failure → Retry → Revert to Host ✅
  - [ ] Watchdog timeout → System reset → Boot into Host ✅

**Deliverable:** All integration tests passing

---

### Task 3.3: Stress Testing (2 hours)

- [ ] **Repeated mode switching**
  - [ ] Switch modes 100 times (button + serial)
  - [ ] Verify no memory leaks (check heap usage)
  - [ ] Verify no USB initialization failures
  - [ ] Verify LED indication remains correct

- [ ] **Power cycle testing**
  - [ ] Power cycle 50 times
  - [ ] Verify default mode (Host) boots correctly
  - [ ] Verify button handler initializes correctly
  - [ ] Verify no boot failures

- [ ] **Long-duration testing**
  - [ ] Run system for 24 hours
  - [ ] Perform mode switches every hour
  - [ ] Verify no crashes, memory leaks, or errors

**Deliverable:** System stable under stress conditions

---

### Task 3.4: Field Testing with Michael (4 hours)

- [ ] **Prepare test environment**
  - [ ] Flash firmware to ESP32-S3 development board
  - [ ] Prepare USB flash drives (various sizes, brands)
  - [ ] Prepare laptop with serial console (PuTTY)
  - [ ] Prepare quick reference card for Michael

- [ ] **Field deployment simulation**
  - [ ] Outdoor environment (if applicable)
  - [ ] With gloves (if applicable)
  - [ ] Low visibility (test LED visibility)
  - [ ] Non-technical user operation (Michael tests)

- [ ] **User acceptance testing**
  - [ ] Michael performs mode switch 10 times (button method)
  - [ ] Michael performs mode switch 5 times (serial command method)
  - [ ] Verify ease of use
  - [ ] Gather feedback
  - [ ] Adjust timeout/LED if needed

- [ ] **Collect feedback**
  - [ ] Is triple-press pattern easy to use?
  - [ ] Is LED indication clear?
  - [ ] Are serial messages helpful?
  - [ ] Any issues or suggestions?

**Deliverable:** User acceptance testing complete, feedback collected

---

### Task 3.5: Documentation (2 hours)

- [ ] **Code documentation**
  - [ ] Add Doxygen comments to all functions
  - [ ] Add file headers with description, author, date
  - [ ] Add inline comments for complex logic
  - [ ] Generate Doxygen HTML documentation

- [ ] **User documentation**
  - [ ] Create quick reference card (1-page PDF)
  - [ ] Update user manual (add mode switching section)
  - [ ] Create troubleshooting guide
  - [ ] Create LED indication reference table

- [ ] **Developer documentation**
  - [ ] Update README.md with mode switching feature
  - [ ] Add architecture diagram (Mermaid)
  - [ ] Add testing procedures
  - [ ] Add known limitations

**Deliverable:** Complete documentation (code + user + developer)

---

### Task 3.6: Bug Fixes and Refinements (2 hours)

- [ ] **Address feedback from field testing**
  - [ ] Fix any bugs discovered during testing
  - [ ] Adjust timeouts if needed (triple-press timeout, debounce time)
  - [ ] Adjust LED brightness if needed
  - [ ] Improve serial messages if needed

- [ ] **Code review and cleanup**
  - [ ] Remove debug code
  - [ ] Remove commented-out code
  - [ ] Fix code style issues (ESP-IDF coding standards)
  - [ ] Run static analysis (cppcheck, clang-tidy)

- [ ] **Final testing**
  - [ ] Run all unit tests ✅
  - [ ] Run all integration tests ✅
  - [ ] Run stress tests ✅
  - [ ] Verify no regressions

**Deliverable:** Production-ready code, all tests passing

---

## Post-Implementation

- [ ] **Merge to develop branch**
  ```bash
  git checkout develop
  git merge feature/usb-mode-switching
  git push origin develop
  ```

- [ ] **Create pull request** (if using GitHub workflow)
  - [ ] Title: "Phase 5: USB Mode Switching - Triple-Press Button + Serial Command"
  - [ ] Description: Link to recommendation document, testing results
  - [ ] Request review from stakeholders

- [ ] **Tag release** (if ready for production)
  ```bash
  git tag -a v2.0.0 -m "Phase 5: USB Mode Switching feature complete"
  git push origin v2.0.0
  ```

- [ ] **Update project documentation**
  - [ ] Update PROJECT_DELIVERY_REPORT.md
  - [ ] Update CHANGELOG.md
  - [ ] Update README.md

- [ ] **Notify stakeholders**
  - [ ] Send email to Michael with release notes
  - [ ] Include quick reference card (PDF)
  - [ ] Include link to GitHub release

---

## Estimated Timeline

| Phase | Tasks | Effort | Calendar Time |
|-------|-------|--------|---------------|
| **Phase 1** | Core mode switching | 10h | 2 days |
| **Phase 2** | Enhanced features | 11h | 2 days |
| **Phase 3** | Testing & validation | 15h | 3 days |
| **TOTAL** | | **36 hours** | **7 days** |

**Note:** Calendar time assumes part-time development (4-6 hours/day)

---

## Success Criteria

- [x] Mode switching works via triple-press button (no tools required)
- [x] Mode switching works via serial command (development/debugging)
- [x] LED indication clear and correct for all modes
- [x] Safety mechanisms prevent accidental switches during USB operations
- [x] Fail-safe behavior ensures system always recovers to Host mode
- [x] All tests passing (unit, integration, stress, field)
- [x] User documentation complete (quick reference card, user manual)
- [x] Code documentation complete (Doxygen comments, README)
- [x] Michael approves feature (user acceptance testing)

---

**Checklist prepared by:** Augment Agent  
**Date:** November 8, 2025  
**Status:** ✅ READY FOR IMPLEMENTATION

---

**Print this checklist and check off items as you complete them!** ✅

