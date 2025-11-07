# 🔧 PowerShell Fix - Build Script Issue

**Problem:** PowerShell doesn't recognize `build.bat` command
**Solution:** Use `.\build.bat` or use PowerShell script instead

---

## ✅ Solution 1: Use `.\` prefix (EASIEST)

### **Instead of:**
```bash
build.bat clean
build.bat build
build.bat flash-both
```

### **Use:**
```bash
.\build.bat clean
.\build.bat build
.\build.bat flash-both
```

**The `.\` tells PowerShell to run the script from current directory**

---

## ✅ Solution 2: Use PowerShell Script (RECOMMENDED)

### **Use build-easy.ps1 instead:**

```bash
.\build-easy.ps1 clean
.\build-easy.ps1 build
.\build-easy.ps1 flash-both
.\build-easy.ps1 monitor-com14
.\build-easy.ps1 monitor-com11
.\build-easy.ps1 full
.\build-easy.ps1 full-both
```

**Same commands, but PowerShell native with colored output!**

---

## ✅ Solution 3: Use CMD Terminal (ALTERNATIVE)

### **Open CMD instead of PowerShell:**

1. Open VS Code Terminal (Ctrl + `)
2. Click dropdown arrow
3. Select "Command Prompt"
4. Then use: `build.bat clean`

**In CMD, you don't need `.\` prefix**

---

## 🎯 RECOMMENDED WORKFLOW

### **Option A: Use PowerShell with `.\` prefix**

```bash
cd c:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host
.\build.bat clean
.\build.bat build
.\build.bat flash-both
.\build.bat monitor-com14
```

### **Option B: Use PowerShell script (build-easy.ps1)**

```bash
cd c:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host
.\build-easy.ps1 clean
.\build-easy.ps1 build
.\build-easy.ps1 flash-both
.\build-easy.ps1 monitor-com14
```

### **Option C: Use CMD terminal**

```bash
cd c:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host
build.bat clean
build.bat build
build.bat flash-both
build.bat monitor-com14
```

---

## 📋 Quick Reference

| Terminal | Command |
|----------|---------|
| **PowerShell** | `.\build.bat clean` |
| **PowerShell** | `.\build-easy.ps1 clean` |
| **CMD** | `build.bat clean` |

---

## 🚀 QUICK START (FIXED)

### **Open VS Code Terminal (Ctrl + `)**

### **Run this command:**
```bash
cd c:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host && .\build.bat clean && .\build.bat build && .\build.bat flash-both && .\build.bat monitor-com14
```

**Or use PowerShell script:**
```bash
cd c:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host && .\build-easy.ps1 clean && .\build-easy.ps1 build && .\build-easy.ps1 flash-both && .\build-easy.ps1 monitor-com14
```

---

## ✅ Available Commands

### **Using build.bat (with `.\` prefix):**
```bash
.\build.bat clean           # Clean build
.\build.bat build           # Build firmware
.\build.bat flash-com14     # Flash to COM14
.\build.bat flash-com11     # Flash to COM11
.\build.bat flash-both      # Flash to BOTH
.\build.bat monitor-com14   # Monitor COM14
.\build.bat monitor-com11   # Monitor COM11
.\build.bat full            # Full workflow (COM14)
.\build.bat full-both       # Full workflow (BOTH)
```

### **Using build-easy.ps1:**
```bash
.\build-easy.ps1 clean           # Clean build
.\build-easy.ps1 build           # Build firmware
.\build-easy.ps1 flash-com14     # Flash to COM14
.\build-easy.ps1 flash-com11     # Flash to COM11
.\build-easy.ps1 flash-both      # Flash to BOTH
.\build-easy.ps1 monitor-com14   # Monitor COM14
.\build-easy.ps1 monitor-com11   # Monitor COM11
.\build-easy.ps1 full            # Full workflow (COM14)
.\build-easy.ps1 full-both       # Full workflow (BOTH)
```

---

## 🎯 RECOMMENDED: Use build-easy.ps1

**Why?**
- ✅ PowerShell native (colored output)
- ✅ Same commands as build.bat
- ✅ Better error handling
- ✅ Easier to use

**How:**
```bash
.\build-easy.ps1 flash-both
.\build-easy.ps1 monitor-com14
```

---

## 📝 Next Steps

1. **Use `.\` prefix with build.bat:**
   ```bash
   .\build.bat clean && .\build.bat build && .\build.bat flash-both
   ```

2. **Or use build-easy.ps1:**
   ```bash
   .\build-easy.ps1 clean && .\build-easy.ps1 build && .\build-easy.ps1 flash-both
   ```

3. **Monitor COM14:**
   ```bash
   .\build.bat monitor-com14
   ```

4. **Exit monitor (Ctrl + ])**

5. **Monitor COM11:**
   ```bash
   .\build.bat monitor-com11
   ```

---

## 🚀 Ready to Test?

**Copy this command:**
```bash
cd c:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host && .\build.bat clean && .\build.bat build && .\build.bat flash-both && .\build.bat monitor-com14
```

**Or use PowerShell script:**
```bash
cd c:\Users\Abdul\Documents\GitHub\esp32s3-michael-usb-host && .\build-easy.ps1 clean && .\build-easy.ps1 build && .\build-easy.ps1 flash-both && .\build-easy.ps1 monitor-com14
```

**Let's test! 🎉**

