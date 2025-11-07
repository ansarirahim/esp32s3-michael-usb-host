# 🌳 Git Branching Strategy - ESP32-S3 USB Host Automator

**Project:** USB Host Mode Automation for Michael Steinmann
**Date:** November 2025
**Strategy:** Git Flow with Feature Branches

---

## 📊 Branch Structure

```
main (production-ready)
  ↑
  └─ develop (integration branch)
       ↑
       ├─ feature/phase-1-led-control
       ├─ feature/phase-2-usb-host
       ├─ feature/phase-3-automator
       ├─ feature/phase-4-testing
       └─ bugfix/* (if needed)
```

---

## 🎯 Branch Naming Convention

### **Main Branches:**
- `main` - Production-ready code (stable releases)
- `develop` - Integration branch (development)

### **Feature Branches:**
- `feature/phase-1-led-control` - Phase 1 implementation
- `feature/phase-2-usb-host` - Phase 2 implementation
- `feature/phase-3-automator` - Phase 3 implementation
- `feature/phase-4-testing` - Phase 4 implementation

### **Bugfix Branches (if needed):**
- `bugfix/issue-description` - Bug fixes

### **Hotfix Branches (if needed):**
- `hotfix/critical-issue` - Critical production fixes

---

## 📋 Workflow for Each Phase

### **Step 1: Create Feature Branch from develop**

```bash
# Make sure develop is up to date
git checkout develop
git pull origin develop

# Create feature branch
git checkout -b feature/phase-1-led-control

# Verify you're on the new branch
git branch -v
```

### **Step 2: Work on Feature**

```bash
# Make changes
# Commit regularly with clear messages
git add .
git commit -m "Phase 1: Implement RMT peripheral driver"
git commit -m "Phase 1: Add LED state machine"
git commit -m "Phase 1: Test LED on GPIO38"
```

### **Step 3: Push Feature Branch**

```bash
# Push to remote
git push -u origin feature/phase-1-led-control

# Verify push
git branch -v
```

### **Step 4: Create Pull Request (PR)**

```bash
# On GitHub/GitLab:
# 1. Go to repository
# 2. Click "New Pull Request"
# 3. Base: develop
# 4. Compare: feature/phase-1-led-control
# 5. Add description
# 6. Create PR
```

### **Step 5: Review & Merge**

```bash
# After review and testing:
git checkout develop
git pull origin develop
git merge --no-ff feature/phase-1-led-control
git push origin develop

# Delete feature branch
git branch -d feature/phase-1-led-control
git push origin --delete feature/phase-1-led-control
```

---

## 🔄 Complete Phase Workflow

### **Phase 1: LED Control**

```bash
# 1. Create branch
git checkout develop
git checkout -b feature/phase-1-led-control

# 2. Implement LED control
# - Implement RMT driver
# - Create LED state machine
# - Test on both boards

# 3. Commit changes
git add .
git commit -m "Phase 1: LED Control - RMT driver implementation"
git commit -m "Phase 1: LED Control - State machine"
git commit -m "Phase 1: LED Control - Testing on COM14 and COM11"

# 4. Push to remote
git push -u origin feature/phase-1-led-control

# 5. Create PR on GitHub
# Base: develop
# Compare: feature/phase-1-led-control

# 6. After review, merge to develop
git checkout develop
git pull origin develop
git merge --no-ff feature/phase-1-led-control
git push origin develop

# 7. Delete feature branch
git branch -d feature/phase-1-led-control
git push origin --delete feature/phase-1-led-control

# 8. Tag release (optional)
git tag -a v1.0.0-phase1 -m "Phase 1: LED Control Complete"
git push origin v1.0.0-phase1
```

### **Phase 2: USB Host Mode**

```bash
# Same workflow as Phase 1
git checkout develop
git checkout -b feature/phase-2-usb-host
# ... implement, commit, push, merge ...
```

### **Phase 3: USB Host Automator**

```bash
# Same workflow as Phase 1
git checkout develop
git checkout -b feature/phase-3-automator
# ... implement, commit, push, merge ...
```

### **Phase 4: Testing & Delivery**

```bash
# Same workflow as Phase 1
git checkout develop
git checkout -b feature/phase-4-testing
# ... implement, commit, push, merge ...
```

---

## 📝 Commit Message Convention

### **Format:**
```
<type>(<scope>): <subject>

<body>

<footer>
```

### **Types:**
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation
- `style:` Code style (formatting, missing semicolons, etc.)
- `refactor:` Code refactoring
- `test:` Adding tests
- `chore:` Build process, dependencies, etc.

### **Examples:**

```bash
# Good commit messages
git commit -m "feat(led): Implement RMT-based WS2812B driver"
git commit -m "feat(led): Add LED state machine with 6 states"
git commit -m "test(led): Add LED control unit tests"
git commit -m "docs(led): Add LED control documentation"
git commit -m "fix(led): Fix LED animation timing issue"
git commit -m "refactor(led): Simplify LED state transitions"
```

---

## 🎯 When to Create Branches

| Situation | Action |
|-----------|--------|
| **Starting new phase** | Create `feature/phase-X-*` branch |
| **Bug found during development** | Create `bugfix/*` branch from develop |
| **Critical production bug** | Create `hotfix/*` branch from main |
| **Code review feedback** | Commit to feature branch, push, update PR |
| **Phase complete** | Merge to develop, delete feature branch |

---

## 🔀 When to Merge

| Condition | Action |
|-----------|--------|
| **Phase implementation complete** | Merge to develop |
| **All tests passing** | Merge to develop |
| **Code review approved** | Merge to develop |
| **All phases complete** | Merge develop → main |
| **Ready for delivery** | Tag release on main |

---

## 📊 Branch Status Commands

```bash
# List all branches
git branch -a

# List branches with last commit
git branch -v

# List branches with tracking info
git branch -vv

# Show branch history
git log --oneline --graph --all

# Show current branch
git branch --show-current

# Show branches merged into current
git branch --merged

# Show branches not merged into current
git branch --no-merged
```

---

## 🚀 Phase 1 Execution Plan

### **Today: Create Phase 1 Branch**

```bash
# 1. Ensure develop is up to date
git checkout develop
git pull origin develop

# 2. Create feature branch
git checkout -b feature/phase-1-led-control

# 3. Verify branch created
git branch -v

# 4. Start implementing Phase 1
# - Implement RMT driver
# - Create LED state machine
# - Test on both boards
```

### **During Phase 1: Regular Commits**

```bash
# After each significant change
git add .
git commit -m "feat(led): Implement RMT peripheral driver"
git commit -m "feat(led): Add LED state machine"
git commit -m "test(led): Test LED on COM14 GPIO38"
git commit -m "test(led): Test LED on COM11 GPIO48"

# Push regularly
git push origin feature/phase-1-led-control
```

### **After Phase 1: Merge to develop**

```bash
# 1. Final commit
git add .
git commit -m "Phase 1: LED Control - Complete implementation"

# 2. Push to remote
git push origin feature/phase-1-led-control

# 3. Create PR on GitHub (optional but recommended)

# 4. Merge to develop
git checkout develop
git pull origin develop
git merge --no-ff feature/phase-1-led-control
git push origin develop

# 5. Delete feature branch
git branch -d feature/phase-1-led-control
git push origin --delete feature/phase-1-led-control

# 6. Tag release
git tag -a v1.0.0-phase1 -m "Phase 1: LED Control Complete"
git push origin v1.0.0-phase1
```

---

## 📈 Project Timeline with Branches

```
Day 1-2: Phase 1 (LED Control)
  └─ feature/phase-1-led-control → develop

Day 2-3: Phase 2 (USB Host Mode)
  └─ feature/phase-2-usb-host → develop

Day 3-4: Phase 3 (USB Host Automator)
  └─ feature/phase-3-automator → develop

Day 4-5: Phase 4 (Testing & Delivery)
  └─ feature/phase-4-testing → develop

Day 5-6: Final Release
  └─ develop → main (v1.0.0)
```

---

## ✅ Best Practices

✅ **Create branch for each phase**
✅ **Commit frequently with clear messages**
✅ **Push regularly to remote**
✅ **Use descriptive branch names**
✅ **Merge with `--no-ff` flag** (preserves branch history)
✅ **Delete merged branches** (keep repo clean)
✅ **Tag releases** (easy to reference)
✅ **Review before merging** (quality control)

---

## 🚀 Ready to Start Phase 1?

**Next command:**
```bash
git checkout develop
git checkout -b feature/phase-1-led-control
git branch -v
```

**Then start implementing LED control!**

