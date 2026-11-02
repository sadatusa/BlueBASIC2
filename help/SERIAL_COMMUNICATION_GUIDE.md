# BlueBASIC Serial Communication & Terminal Upload Guide

**Target Hardware:** CC2541 HM-10 Module  
**Interface:** UART1 Alternative 2 (`P1_6` = TX, `P1_7` = RX)  
**Default Parameters:** 9600 Baud, 8 Data Bits, No Parity, 1 Stop Bit (8-N-1), No Flow Control  

---

## 1. Overview

This document provides instructions for communicating with BlueBASIC on the CC2541 HM-10 module via serial terminals (e.g., Tera Term, CoolTerm, RealTerm, PuTTY, Arduino IDE).

It covers two primary operating modes:
1. **Interactive Manual Typing:** Typing commands directly keystroke-by-keystroke.
2. **Program File Uploading:** Transferring `.bas` code files without dropping characters.

---

## 2. Interactive Manual Typing

You can connect with any standard serial terminal and type commands directly at the BlueBASIC prompt (`OK`).

### Why Direct Typing Is 100% Reliable:
- **Typing Speed:** Normal human typing (~100 to 300 ms between keystrokes) is well within the CC2541's UART processing capabilities.
- **Clock Stability:** The radio-halt clock divider has been disabled (`HCI_EXT_DISABLE_CLK_DIVIDE_ON_HALT`), preventing the CC2541 system clock from down-clocking during Bluetooth operations. The baud rate generator remains stable at 9600 baud.
- **Power State:** At boot, BlueBASIC holds an active power state (`PWRMGR_HOLD`) to prevent premature sleep while communicating.

---

## 3. Program File Upload & Timing Requirements

When uploading an entire `.bas` program file, software terminals attempt to stream characters continuously at line rate. Because the CC2541 is an 8051 microcontroller running at 32 MHz that must **tokenize each line and commit bytecode into flash memory**, sending unpaced text will overflow the UART buffer and drop characters.

### Required Pacing Delays:

| Parameter | Recommended Setting | Minimum Safe Setting | Purpose |
| :--- | :--- | :--- | :--- |
| **Character Delay** (`msec/char`) | **`5 ms`** | `2 ms` | Prevents overflowing the CC2541 UART FIFO buffer. |
| **Line Delay** (`msec/line`) | **`100 ms`** | `60 ms` | Allows BlueBASIC to parse the line, commit bytecode to flash memory, re-sort line pointers, and return to input mode. |

---

## 4. Terminal Configuration Instructions

### A. Tera Term (Recommended for Windows)

1. Open **Tera Term** and select **Serial** $\rightarrow$ your COM port (e.g., `COM1`).
2. Go to **Setup** $\rightarrow$ **Serial port...**:
   - **Baud rate:** `9600`
   - **Data:** `8 bit`
   - **Parity:** `none`
   - **Stop:** `1 bit`
   - **Flow control:** `none`
   - **Transmit delay:**
     - `msec/char`: **`5`**
     - `msec/line`: **`100`**
3. Click **OK**.
4. Go to **Setup** $\rightarrow$ **Terminal...**:
   - **Receive:** `CR+LF` or `AUTO`
   - **Transmit:** `CR+LF`
5. **To Upload a Program File:**
   - Type `NEW` and press **Enter**.
   - Click **File** $\rightarrow$ **Send file...**.
   - Select your `.bas` file and click **Open**.
   - After the upload completes, type `LIST` to verify the program, then `RUN`.

---

### B. CoolTerm

1. Click **Options** $\rightarrow$ **Serial Port**:
   - **Port:** Select your COM port
   - **Baudrate:** `9600`
   - **Data Bits:** `8`, **Parity:** `None`, **Stop Bits:** `1`
2. In the same Options window, navigate to **Transmit**:
   - Check **Use character delay** $\rightarrow$ set to **`5 ms`**.
   - Check **Use line delay** $\rightarrow$ set to **`100 ms`**.
3. Navigate to **Terminal**:
   - **Enter Key Emulation:** `CR+LF`
4. Click **OK** and then click **Connect**.
5. **To Upload a Program File:**
   - Type `NEW` and press **Enter**.
   - Click **Connection** $\rightarrow$ **Send Text File...**.
   - Select your `.bas` file.

---

### C. RealTerm

1. Open the **Port** tab:
   - **Baud:** `9600`
   - **Port:** Select your COM port
   - Click **Open** (and **Change** if updating settings).
2. Open the **Send** tab:
   - In the **Delays** section:
     - **Char Delay:** set to **`5`**
     - **Line Delay:** set to **`100`**
3. **To Upload a Program File:**
   - In the **Dump File to Port** section, click the `...` button to choose your `.bas` file.
   - Click **Send File**.

---

### D. PuTTY

> [!WARNING]
> PuTTY does not offer built-in pacing delays for pasting or uploading files. Pasting large blocks of text all at once will cause characters to be dropped.
> 
> - For **manual interaction**, PuTTY works reliably.
> - For **file uploads**, use **Tera Term** or **CoolTerm** with the pacing delays configured above.
> - If you must paste in PuTTY, paste small sections of 3–5 lines at a time.

---

### E. Arduino Serial Monitor

1. Select your COM port in the Arduino IDE.
2. Open the **Serial Monitor**.
3. Set the bottom-right dropdowns:
   - **Baud rate:** `9600 baud`
   - **Line ending:** `Both NL & CR`
4. Type commands into the top send bar and press **Enter**.
5. Direct typing works as expected. For multi-line file upload, use Tera Term or CoolTerm.

---

## 5. Automated Upload via PowerShell Script

If you want to upload `.bas` files from the command line without installing third-party terminals, use this PowerShell helper script:

```powershell
param(
    [string]$PortName = "COM1",
    [string]$FilePath = "program.bas"
)

$port = New-Object System.IO.Ports.SerialPort $PortName, 9600, 'None', 8, 'One'
$port.ReadTimeout = 500
$port.WriteTimeout = 1000
$port.DtrEnable = $true
$port.RtsEnable = $true
$port.Open()
Start-Sleep -Milliseconds 300
$port.DiscardInBuffer()

function Send-Line($line) {
    # Send characters with 5ms spacing
    foreach ($c in $line.ToCharArray()) {
        $port.Write($c.ToString())
        [System.Threading.Thread]::Sleep(5)
    }
    $port.Write("`r`n")
    [System.Threading.Thread]::Sleep(100) # 100ms line delay
}

Write-Host "Resetting interpreter..."
Send-Line "NEW"

Write-Host "Uploading $FilePath..."
$lines = Get-Content $FilePath
foreach ($line in $lines) {
    if ($line.Trim().Length -gt 0) {
        Send-Line $line
        Write-Host "Uploaded: $line"
    }
}

Write-Host "Upload complete. Checking listing:"
Send-Line "LIST"
Start-Sleep -Milliseconds 500
Write-Host $port.ReadExisting()

$port.Close()
```

---

## 6. BlueBASIC Command Quick Reference

| Command | Action |
| :--- | :--- |
| `NEW` | Clears current program lines from flash memory and resets line pointers. |
| `LIST` | Prints all stored program lines in ascending order. |
| `RUN` | Executes the program starting from the lowest line number. |
| `MEM` | Displays remaining free bytes in the flash/heap space. |
| `REBOOT` | Reboots the CC2541 module. |
| `WAKEUP` | Disables power-saving sleep mode and locks CPU in active mode. |
| `POWERSAVING` | Re-enables low-power PM2 sleep when idle. |

### Editing Lines:
- To **add** a line: Type `<linenum> <statement>` and press Enter (e.g., `10 PRINT "HELLO"`).
- To **replace** a line: Re-enter the same line number with the new statement.
- To **delete** a line: Type `<linenum>` with nothing after it and press Enter (e.g., `10`).
