# Install Pandoc - Quick Guide

**Purpose:** Convert Phase 5 Markdown documentation to PDF

---

## 🚀 Quick Install (Windows)

### **Option 1: Chocolatey (Recommended)**

**1. Install Chocolatey (if not installed):**

Open PowerShell as Administrator and run:
```powershell
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
```

**2. Install Pandoc:**
```powershell
choco install pandoc
```

**3. Verify:**
```powershell
pandoc --version
```

---

### **Option 2: Manual Download (Easiest)**

**1. Download Pandoc:**
- Go to: https://github.com/jgm/pandoc/releases/latest
- Download: `pandoc-3.x.x-windows-x86_64.msi`

**2. Install:**
- Double-click the `.msi` file
- Follow installation wizard
- Click "Next" → "Next" → "Install"

**3. Verify:**
Open new PowerShell window:
```powershell
pandoc --version
```

---

### **Option 3: Scoop**

**1. Install Scoop (if not installed):**
```powershell
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
irm get.scoop.sh | iex
```

**2. Install Pandoc:**
```powershell
scoop install pandoc
```

**3. Verify:**
```powershell
pandoc --version
```

---

## 📦 After Installation

### **Convert Phase 5 Documents:**

**Method 1: Batch File (Easiest)**
```batch
convert_phase5_to_pdf.bat
```

**Method 2: PowerShell Script**
```powershell
powershell -ExecutionPolicy Bypass -File scripts\convert_phase5_to_pdf.ps1
```

**Method 3: Manual (Single File)**
```powershell
pandoc PHASE5_EXECUTIVE_SUMMARY.md -o Phase5_Executive_Summary.pdf --pdf-engine=xelatex --toc
```

---

## 🔧 Troubleshooting

### **Error: "xelatex not found"**

**Solution:** Install MiKTeX (LaTeX distribution)

```powershell
choco install miktex
```

Or download manually: https://miktex.org/download

---

### **Alternative: Use Online Converter**

If you don't want to install pandoc:

1. **Go to:** https://www.markdowntopdf.com/
2. **Upload:** `PHASE5_EXECUTIVE_SUMMARY.md`
3. **Download:** PDF
4. **Repeat** for other files

---

## ✅ Ready!

Once pandoc is installed, run:
```batch
convert_phase5_to_pdf.bat
```

PDFs will be generated in `docs/phase5_pdf/`

