# BlueBASIC CC2541 Hardware I2C (`HWI2C`) Examples Guide

**Target Hardware:** Texas Instruments CC2541 Bluetooth Low Energy SoC  
**Firmware:** BlueBASIC v1.4.0 (with Native Hardware I2C Engine)  
**Hardware Pins:** Dedicated **Pin 2 (SCL)** and **Pin 3 (SDA)** with 4.7 kΩ pull-ups to 3.3V  

---

## 1. Quick Overview & Hardware Pinout

The **CC2541** features a built-in hardware I2C master controller. Unlike the software bit-bang engine (`I2C` / `SWI2C`) which runs on arbitrary GPIO pins, `HWI2C` uses the chip's dedicated hardware peripheral pins:

| Signal | CC2541 QFN-40 Pin | Typical Module Pin | Function |
| :--- | :--- | :--- | :--- |
| **SCL** | **Pin 2** | Clock | Hardware I2C Serial Clock |
| **SDA** | **Pin 3** | Data | Hardware I2C Serial Data |
| **VCC** | VDD | 3.3V | Module Power Supply (3.3V) |
| **GND** | GND | GND | Common Ground |

> [!IMPORTANT]
> **CC2540 Target Safety:**  
> The CC2540 does not have this hardware peripheral (pins 2 and 3 are USB). Executing `HWI2C` on CC2540 automatically returns:
> ```text
> HWI2C not Available
> ```

---

## 2. Command Quick Reference

```basic
REM 1. Initialize Hardware Master (Default: 123 kHz, or specify speed in kHz)
HWI2C MASTER [33 | 100 | 250 | 400]

REM 2. Write Data Bytes (Supports 7-bit e.g. 104, 87 or 8-bit e.g. 208, 174)
HWI2C WRITE <address>, <byte1>, <byte2>, ...

REM 3. Read Data Bytes into Variable or DIM Array
HWI2C READ <address>, <var>
HWI2C READ <address>, <dim_array>

REM 4. Combined Write-then-Read (Random-Access Register Read)
HWI2C WRITE <address>, <reg_offset>, READ <var> | <dim_array>

REM 5. Close / Stop Hardware I2C Controller
HWI2C STOP
HWI2C CLOSE
```

---

## 3. Example 1 [Real Hardware Tested]: ZS-042 (DS3231 RTC + HK24C32 EEPROM) Complete Hardware Test

> [!NOTE]
> **Status: Real Hardware Tested** on physical CC2541 (HM-10) connected to ZS-042 (DS3231 RTC + HK24C32 EEPROM) over dedicated hardware I2C pins (Pin 2 SCL, Pin 3 SDA) via UART COM1.

This program is fully verified on real CC2541 hardware connected to a ZS-042 module. It reads the real-time clock, reads the internal die temperature sensor, and tests EEPROM 16-bit word address writing and reading.

```basic
10 REM ========================================================
20 REM CC2541 HARDWARE I2C (HWI2C) REAL TEST WITH ZS-042
30 REM ========================================================
40 HWI2C MASTER 100
50 PRINT "========================================"
60 PRINT "CC2541 HARDWARE I2C REAL TEST: DS3231 + 24C32"
70 PRINT "========================================"
80 REM 1. READ TIME FROM DS3231 (ADDRESS 208 = 0XD0)
90 DIM D(7)
100 HWI2C WRITE 208, 0, READ D
110 S = (((D(0) & 127) >> 4) * 10) + (D(0) & 15)
120 M = ((D(1) >> 4) * 10) + (D(1) & 15)
130 H = (((D(2) & 63) >> 4) * 10) + (D(2) & 15)
140 PRINT "CURRENT TIME:  ", H, ":", M, ":", S
150 REM 2. READ DIE TEMPERATURE
160 DIM T(2)
170 HWI2C WRITE 208, 17, READ T
180 C = T(0)
190 F = (T(1) >> 6) * 25
200 PRINT "DIE TEMP:      ", C, ".", F, " C"
210 REM 3. WRITE AND READ HK24C32 EEPROM
220 PRINT "--- EEPROM 16-BIT WORD ADDRESS TEST ---"
230 HWI2C WRITE 174, 0, 64, 72, 73, 74
240 FOR I = 1 TO 400
245 NEXT I
250 DIM E(3)
260 HWI2C WRITE 174, 0, 64, READ E
270 PRINT "EEPROM READ:   ", E(0), " ", E(1), " ", E(2)
280 X = (E(0) == 72)
290 Y = (E(1) == 73)
300 Z = (E(2) == 74)
310 IF X & Y & Z
320 PRINT "VERIFICATION:  PASSED (ALL BYTES MATCH)!"
330 ELSE
340 PRINT "VERIFICATION:  FAILED!"
350 END
360 PRINT "========================================"
370 RETURN
```

### Verified Terminal Output on Real Hardware (`RUN`):
```text
RUN
========================================
CC2541 HARDWARE I2C REAL TEST: DS3231 + 24C32
========================================
CURRENT TIME:  13:0:12
DIE TEMP:      29.75 C
--- EEPROM 16-BIT WORD ADDRESS TEST ---
EEPROM READ:   72 73 74
VERIFICATION:  PASSED (ALL BYTES MATCH)!
========================================
OK
```

---

## 4. Example 2 [Real Hardware Tested]: Interactive Direct Mode (Quick One-Liners)

> [!NOTE]
> **Status: Real Hardware Tested** directly at the BlueBASIC interactive terminal prompt on CC2541 hardware over COM1.

Unlike software I2C (which requires stored program lines), `HWI2C` supports direct execution from the terminal:

### Read DS3231 Time Directly:
```basic
HWI2C MASTER 100
DIM D(7)
HWI2C WRITE 208, 0, READ D
PRINT (((D(2) & 63) >> 4) * 10) + (D(2) & 15), ":", ((D(1) >> 4) * 10) + (D(1) & 15), ":", (((D(0) & 127) >> 4) * 10) + (D(0) & 15)
```

### Read DS3231 Temperature Directly:
```basic
DIM T(2)
HWI2C WRITE 208, 17, READ T
PRINT "Temperature: ", T(0), " C"
```

### Read Single EEPROM Byte Directly:
```basic
DIM B(1)
HWI2C WRITE 174, 0, 64, READ B
PRINT "Byte at 0x0040: ", B(0)
```

---

## 5. Example 3: Continuous RTC Clock Reader (Polling Loop)

A program that reads the DS3231 every second and prints an updated digital clock display:

```basic
10 REM ========================================================
20 REM DS3231 CONTINUOUS REAL-TIME CLOCK DISPLAY
30 REM ========================================================
40 HWI2C MASTER 100
50 DIM D(7)
60 PRINT "=== DS3231 DIGITAL CLOCK (CTRL+C TO STOP) ==="
70 HWI2C WRITE 208, 0, READ D
80 S = (((D(0) & 127) >> 4) * 10) + (D(0) & 15)
90 M = ((D(1) >> 4) * 10) + (D(1) & 15)
100 H = (((D(2) & 63) >> 4) * 10) + (D(2) & 15)
110 PRINT H, ":", M, ":", S
120 FOR I = 1 TO 10000
130 NEXT I
140 GOTO 70
```

---

## 6. Example 4 [Real Hardware Tested]: HK24C32 / AT24C32 16-Bit Word EEPROM Reader & Writer

> [!NOTE]
> **Status: Real Hardware Tested** with 16-bit word addresses and synchronous write cycle delays (`FOR I = 1 TO 400: NEXT I`) on CC2541 hardware.

The 24C32 EEPROM has 4,096 bytes ($0x0000$ to $0x0FFF$) and requires a **16-bit word address** (`address_high`, `address_low`):

```basic
10 REM ========================================================
20 REM 24C32 EEPROM 8-BYTE BLOCK WRITE AND READ
30 REM ========================================================
40 HWI2C MASTER 100
50 REM WRITE 8 BYTES TO EEPROM ADDRESS 0X0100 (HIGH=1, LOW=0)
60 PRINT "WRITING 8 BYTES TO 0X0100..."
70 HWI2C WRITE 174, 1, 0, 10, 20, 30, 40, 50, 60, 70, 80
80 REM WAIT FOR INTERNAL EEPROM WRITE CYCLE (tWR approx 5ms)
90 FOR I = 1 TO 400
100 NEXT I
110 REM READ 8 BYTES BACK FROM 0X0100
120 DIM R(8)
130 HWI2C WRITE 174, 1, 0, READ R
140 PRINT "READ BACK:"
150 FOR I = 0 TO 7
160 PRINT "BYTE ", I, ": ", R(I)
170 NEXT I
180 RETURN
```

---

## 7. Example 5 [Real Hardware Tested]: AHT20 Humidity & Temperature Sensor

> [!NOTE]
> **Status: Real Hardware Tested** on physical CC2541 hardware connected to AHT20 via dedicated hardware I2C pins (Pin 2 SCL, Pin 3 SDA) over UART COM1.

The **AHT20** is an industrial-grade digital relative humidity and temperature sensor. It communicates at 7-bit I2C address `0x38` (decimal `56`).

### Protocol Flow:
1. **Initialize/Calibration Check:** Read status byte via `HWI2C READ 56, S`. If bit 3 is `0`, send `HWI2C WRITE 56, 190, 8, 0` and delay 10ms.
2. **Trigger Measurement:** Send `0xAC, 0x33, 0x00` (`172, 51, 0`).
3. **Wait Measurement:** Delay ~75-80ms (`FOR I = 1 TO 3500: NEXT I`).
4. **Read Data:** Read 7 bytes (`HWI2C READ 56, D`). Byte 0 is Status, Bytes 1-3 are 20-bit Humidity, Bytes 3-5 are 20-bit Temperature.
5. **Convert:**
   - $\text{RH} = \frac{\text{HRAW} \times 100}{1048576}$
   - $T(^\circ\text{C}) = \frac{\text{TRAW} \times 200}{1048576} - 50$

```basic
10 REM ========================================================
20 REM CC2541 HARDWARE I2C (HWI2C) AHT20 TEMPERATURE & HUMIDITY
30 REM ========================================================
40 HWI2C MASTER 100
50 PRINT "========================================"
60 PRINT "AHT20 TEMPERATURE & HUMIDITY TEST"
70 PRINT "========================================"
80 REM 1. CHECK STATUS / INITIALIZE IF NEEDED
90 DIM S(1)
100 HWI2C READ 56, S
110 IF (S(0) & 8) == 0
120 HWI2C WRITE 56, 190, 8, 0
130 FOR I = 1 TO 500
135 NEXT I
140 END
150 REM 2. TRIGGER MEASUREMENT (0xAC, 0x33, 0x00)
160 HWI2C WRITE 56, 172, 51, 0
170 FOR I = 1 TO 3500
175 NEXT I
180 REM 3. READ 7 BYTES (STATUS, HUMIDITY, TEMP, CRC)
190 DIM D(7)
200 HWI2C READ 56, D
210 IF (D(0) & 128)
220 PRINT "AHT20 BUSY, PLEASE RETRY"
230 RETURN
240 END
250 REM 4. CALCULATE HUMIDITY (20-BIT)
260 H = (D(1) << 12) | (D(2) << 4) | (D(3) >> 4)
270 R = (H * 100) / 1048576
280 F = ((H * 1000) / 1048576) % 10
290 PRINT "RELATIVE HUMIDITY: ", R, ".", F, " %"
300 REM 5. CALCULATE TEMPERATURE (20-BIT)
310 T = ((D(3) & 15) << 16) | (D(4) << 8) | D(5)
320 X = ((T * 2000) / 1048576) - 500
330 C = X / 10
335 M = X % 10
340 IF M < 0
342 M = 0 - M
345 END
350 PRINT "TEMPERATURE:       ", C, ".", M, " C"
360 PRINT "========================================"
370 RETURN
```

### Verified Terminal Output on Real Hardware (`RUN`):
```text
RUN
========================================
AHT20 TEMPERATURE & HUMIDITY TEST
========================================
RELATIVE HUMIDITY: 47.8 %
TEMPERATURE:       30.2 C
========================================
OK
```

---

## 7B. Example 5B [Real Hardware Tested]: AHT10 Humidity & Temperature Sensor

> [!NOTE]
> **Status: Real Hardware Tested** on physical CC2541 hardware connected to an AHT10 sensor via dedicated hardware I2C pins (Pin 2 SCL, Pin 3 SDA) over UART COM1.

The **AHT10** is an Aosong digital relative humidity and temperature sensor. It operates at 7-bit I2C address `0x38` (decimal `56`).

### Protocol Flow:
1. **Initialize/Calibration Check:** Read status byte via `HWI2C READ 56, S`. If bit 3 is `0` (uncalibrated), send `HWI2C WRITE 56, 225, 8, 0` (`0xE1, 0x08, 0x00`) and delay 50ms.
2. **Trigger Measurement:** Send `0xAC, 0x33, 0x00` (`172, 51, 0`).
3. **Wait Measurement:** Delay ~100-120ms (`DELAY 120`).
4. **Read Data:** Read 6 bytes (`HWI2C READ 56, D`). Byte 0 is Status, Bytes 1-3 are 20-bit Humidity, Bytes 3-5 are 20-bit Temperature.
5. **Convert (32-bit Integer Math):**
   - $\text{RH} = \frac{\text{HRAW} \times 100}{1048576}$
   - $T(^\circ\text{C}) = \frac{\text{TRAW} \times 200}{1048576} - 50$

```basic
10 REM ========================================================
15 REM STATUS: REAL HARDWARE TESTED (VCC = 3.3V, CC2541 HWI2C)
20 REM AHT10 HARDWARE I2C (HWI2C) TEMPERATURE & HUMIDITY DRIVER
25 REM VERIFIED ON REAL HARDWARE OVER COM1 AT 9600 BAUD
30 REM ========================================================
40 HWI2C MASTER 100
50 PRINT "========================================"
60 PRINT "  AHT10 SENSOR TEST (HWI2C ADDR 56)"
70 PRINT "========================================"
80 REM --- STEP 1: VERIFY CALIBRATION STATUS ---
90 DIM S(1)
100 HWI2C READ 56, S
110 IF (S(0) & 8) == 0
120   PRINT "AHT10 NOT CALIBRATED. SENDING INIT..."
130   HWI2C WRITE 56, 225, 8, 0
140   DELAY 50
150 END
160 PRINT "AHT10 SENSOR INITIALIZED AND READY!"
170 DELAY 100
180 PRINT "----------------------------------------"
190 DIM D(6)
200 FOR N = 1 TO 5
210   REM --- STEP 2: TRIGGER MEASUREMENT ---
220   HWI2C WRITE 56, 172, 51, 0
230   DELAY 120
240   REM --- STEP 3: READ 6 DATA BYTES ---
250   HWI2C READ 56, D
260   IF (D(0) & 128)
270     PRINT "READING #"; N; " | SENSOR BUSY"
280   END
290   IF (D(0) & 128) == 0
300     REM --- STEP 4: 20-BIT HUMIDITY CALCULATION ---
310     H = (D(1) << 12) | (D(2) << 4) | (D(3) >> 4)
320     R = (H * 100) / 1048576
330     F = ((H * 1000) / 1048576) % 10
340     REM --- STEP 5: 20-BIT TEMPERATURE CALCULATION ---
350     T = ((D(3) & 15) << 16) | (D(4) << 8) | D(5)
360     X = ((T * 2000) / 1048576) - 500
370     C = X / 10
380     M = X % 10
390     IF M < 0
400       M = 0 - M
410     END
420     PRINT "READING #"; N; " | HUMIDITY: "; R; "."; F; " % | TEMP: "; C; "."; M; " C"
430   END
440   DELAY 1000
450 NEXT N
460 PRINT "========================================"
470 PRINT "AHT10 TEST COMPLETED SUCCESSFULLY!"
480 PRINT "========================================"
```

### Verified Terminal Output on Real Hardware (`RUN`):
```text
RUN
========================================
  AHT10 SENSOR TEST (HWI2C ADDR 56)
========================================
AHT10 SENSOR INITIALIZED AND READY!
----------------------------------------
READING #1 | HUMIDITY: 42.7 % | TEMP: 30.0 C
READING #2 | HUMIDITY: 42.7 % | TEMP: 30.0 C
READING #3 | HUMIDITY: 42.7 % | TEMP: 30.0 C
READING #4 | HUMIDITY: 42.7 % | TEMP: 30.0 C
READING #5 | HUMIDITY: 42.7 % | TEMP: 30.0 C
========================================
AHT10 TEST COMPLETED SUCCESSFULLY!
========================================
OK
```

---

## 8. Example 6 [Real Hardware Tested]: BMP280 Atmospheric Pressure & Precision Temperature Sensor

> [!NOTE]
> **Status: Real Hardware Tested** on physical CC2541 hardware connected to BMP280 at 8-bit wire address `238` (`0xEE`, SDO=VDD) over dedicated hardware I2C pins.

The **BMP280** is a high-precision digital barometric pressure and temperature sensor. It provides factory calibration parameters (`dig_T1..dig_T3`, `dig_P1..dig_P9`) stored in non-volatile registers `0x88` to `0xA1` (24 bytes).

### Conversion Architecture:
This BlueBASIC driver implements the official Bosch 32-bit fixed-point compensation algorithm adapted for BlueBASIC's 32-bit signed integer math:
- Factory calibration loaded directly into `DIM C(24)`.
- Live measurement read from registers `0xF7` to `0xFC` into `DIM M(6)`.
- Full Little-Endian 16-bit signed sign-extension (`IF C(x) & 128: V = V - 65536: END`).
- Full Bosch 32-bit compensation generating exact atmospheric pressure in Pascals and hectopascals (hPa).

```basic
10 REM ========================================================
20 REM CC2541 HARDWARE I2C (HWI2C) BMP280 PRESSURE & TEMP
30 REM ========================================================
40 HWI2C MASTER 100
50 PRINT "========================================"
60 PRINT "BMP280 ATMOSPHERIC PRESSURE & TEMP TEST"
70 PRINT "========================================"
80 REM 1. VERIFY CHIP ID AT REG 208 (0xD0) = 88 (0x58)
90 DIM M(6)
100 HWI2C WRITE 238, 208, READ M
110 IF M(0) != 88
120 PRINT "BMP280 NOT FOUND! ID=", M(0)
130 RETURN
140 END
150 REM 2. CONFIGURE NORMAL MODE & OVERSAMPLING
160 HWI2C WRITE 238, 244, 39
170 HWI2C WRITE 238, 245, 0
180 REM 3. READ 24 CALIBRATION BYTES (REG 136 = 0x88)
190 DIM C(24)
200 HWI2C WRITE 238, 136, READ C
210 REM 4. READ 6 MEASUREMENT BYTES (REG 247 = 0xF7)
220 HWI2C WRITE 238, 247, READ M
240 T = (M(3) << 12) | (M(4) << 4) | (M(5) >> 4)
250 P = (M(0) << 12) | (M(1) << 4) | (M(2) >> 4)
260 REM 5. TEMPERATURE COMPENSATION
270 U = C(0) | (C(1) << 8)
280 V = C(2) | (C(3) << 8)
290 IF C(3) & 128
295 V = V - 65536
300 END
310 W = C(4) | (C(5) << 8)
320 IF C(5) & 128
325 W = W - 65536
330 END
340 X = (T >> 3) - (U << 1)
345 X = (X * V) >> 11
350 Y = (T >> 4) - U
360 Y = (Y * Y) >> 12
365 Y = (Y * W) >> 14
370 F = X + Y
380 D = (F * 5 + 128) >> 8
390 I = D / 100
400 J = D % 100
410 IF J < 0
415 J = 0 - J
420 END
430 PRINT "TEMPERATURE:       ", I, ".", J, " C"
440 REM 6. PRESSURE COMPENSATION
450 A = (F >> 1) - 64000
460 K = C(16) | (C(17) << 8)
470 IF C(17) & 128
475 K = K - 65536
480 END
485 E = A >> 2
490 B = ((E * E) >> 11) * K
500 L = C(14) | (C(15) << 8)
510 IF C(15) & 128
515 L = L - 65536
520 END
530 B = B + ((A * L) << 1)
540 N = C(12) | (C(13) << 8)
550 IF C(13) & 128
555 N = N - 65536
560 END
570 B = (B >> 2) + (N << 16)
580 O = C(10) | (C(11) << 8)
590 IF C(11) & 128
595 O = O - 65536
600 END
610 Q = C(8) | (C(9) << 8)
620 IF C(9) & 128
625 Q = Q - 65536
630 END
635 E = A >> 2
637 Z = (E * E) >> 13
640 A = (((O * Z) >> 3) + ((Q * A) >> 1)) >> 18
650 R = C(6) | (C(7) << 8)
660 A = ((32768 + A) * R) >> 15
670 IF A == 0
675 PRINT "PRESSURE ERROR DIV 0"
680 RETURN
685 END
686 E = B >> 12
688 S = 1048576 - P - E
690 S = S * 3125
700 IF S < 1073741824
705 S = (S << 1) / A
710 ELSE
715 S = (S / A) * 2
720 END
730 U = C(22) | (C(23) << 8)
740 IF C(23) & 128
745 U = U - 65536
750 END
755 E = S >> 3
757 Z = (E * E) >> 13
760 A = (U * Z) >> 12
770 V = C(20) | (C(21) << 8)
780 IF C(21) & 128
785 V = V - 65536
790 END
800 B = ((S >> 2) * V) >> 13
810 W = C(18) | (C(19) << 8)
820 IF C(19) & 128
825 W = W - 65536
830 END
840 S = S + ((A + B + W) >> 4)
850 H = S / 100
860 G = S % 100
870 IF G < 0
875 G = 0 - G
880 END
890 PRINT "PRESSURE:          ", H, ".", G, " hPa (", S, " Pa)"
900 PRINT "========================================"
910 RETURN
```

### Verified Terminal Output on Real Hardware (`RUN`):
```text
RUN
========================================
BMP280 ATMOSPHERIC PRESSURE & TEMP TEST
========================================
TEMPERATURE:       31.58 C
PRESSURE:          1009.61 hPa (100961 Pa)
========================================
OK
```

---

## 9. Example 7 [Real Hardware Tested]: Combined Weather Station (AHT20 + BMP280)

> [!NOTE]
> **Status: Real Hardware Tested** with both AHT20 and BMP280 operating simultaneously on the CC2541 hardware I2C bus.

This program reads **Relative Humidity** and **Temperature** from the AHT20, then reads **Barometric Pressure** and **Precision Temperature** from the BMP280, providing a complete environmental telemetry station in pure BlueBASIC.

```basic
10 REM ========================================================
20 REM CC2541 HARDWARE I2C WEATHER STATION: AHT20 + BMP280
30 REM ========================================================
40 HWI2C MASTER 100
50 PRINT "========================================"
60 PRINT "CC2541 WEATHER STATION (AHT20 + BMP280)"
70 PRINT "========================================"
80 REM ------------------------------------------------
90 REM 1. READ AHT20 (HUMIDITY & TEMP) AT ADDR 56 (0x38)
100 REM ------------------------------------------------
110 DIM M(7)
120 HWI2C WRITE 56, 172, 51, 0
130 FOR I = 1 TO 3500
135 NEXT I
140 HWI2C READ 56, M
150 H = (M(1) << 12) | (M(2) << 4) | (M(3) >> 4)
160 R = (H * 100) / 1048576
170 F = ((H * 1000) / 1048576) % 10
180 T = ((M(3) & 15) << 16) | (M(4) << 8) | M(5)
190 X = ((T * 2000) / 1048576) - 500
200 D = X / 10
210 J = X % 10
220 IF J < 0
225 J = 0 - J
230 END
240 PRINT "AHT20 HUMIDITY:    ", R, ".", F, " %"
250 PRINT "AHT20 TEMPERATURE: ", D, ".", J, " C"
260 REM ------------------------------------------------
270 REM 2. READ BMP280 (PRESSURE & TEMP) AT ADDR 238 (0xEE)
280 REM ------------------------------------------------
290 HWI2C WRITE 238, 244, 39
300 HWI2C WRITE 238, 245, 0
310 DIM C(24)
320 HWI2C WRITE 238, 136, READ C
340 HWI2C WRITE 238, 247, READ M
350 T = (M(3) << 12) | (M(4) << 4) | (M(5) >> 4)
360 P = (M(0) << 12) | (M(1) << 4) | (M(2) >> 4)
370 U = C(0) | (C(1) << 8)
380 V = C(2) | (C(3) << 8)
390 IF C(3) & 128
395 V = V - 65536
400 END
410 W = C(4) | (C(5) << 8)
420 IF C(5) & 128
425 W = W - 65536
430 END
440 X = (T >> 3) - (U << 1)
450 X = (X * V) >> 11
460 Y = (T >> 4) - U
470 Y = (Y * Y) >> 12
480 Y = (Y * W) >> 14
490 F = X + Y
500 D = (F * 5 + 128) >> 8
510 I = D / 100
520 J = D % 100
530 IF J < 0
535 J = 0 - J
540 END
550 PRINT "BMP280 TEMPERATURE:", I, ".", J, " C"
560 A = (F >> 1) - 64000
570 K = C(16) | (C(17) << 8)
580 IF C(17) & 128
585 K = K - 65536
590 END
595 E = A >> 2
600 B = ((E * E) >> 11) * K
610 L = C(14) | (C(15) << 8)
620 IF C(15) & 128
625 L = L - 65536
630 END
640 B = B + ((A * L) << 1)
650 N = C(12) | (C(13) << 8)
660 IF C(13) & 128
665 N = N - 65536
670 END
680 B = (B >> 2) + (N << 16)
690 O = C(10) | (C(11) << 8)
700 IF C(11) & 128
705 O = O - 65536
710 END
720 Q = C(8) | (C(9) << 8)
730 IF C(9) & 128
735 Q = Q - 65536
740 END
745 E = A >> 2
750 Z = (E * E) >> 13
755 A = (((O * Z) >> 3) + ((Q * A) >> 1)) >> 18
760 R = C(6) | (C(7) << 8)
765 A = ((32768 + A) * R) >> 15
770 IF A == 0
775 PRINT "PRESSURE ERROR DIV 0"
780 RETURN
785 END
790 E = B >> 12
795 S = 1048576 - P - E
800 S = S * 3125
810 IF S < 1073741824
815 S = (S << 1) / A
820 ELSE
825 S = (S / A) * 2
830 END
840 U = C(22) | (C(23) << 8)
850 IF C(23) & 128
855 U = U - 65536
860 END
865 E = S >> 3
870 Z = (E * E) >> 13
875 A = (U * Z) >> 12
880 V = C(20) | (C(21) << 8)
890 IF C(21) & 128
895 V = V - 65536
900 END
910 B = ((S >> 2) * V) >> 13
920 W = C(18) | (C(19) << 8)
930 IF C(19) & 128
935 W = W - 65536
940 END
950 S = S + ((A + B + W) >> 4)
960 H = S / 100
970 G = S % 100
980 IF G < 0
985 G = 0 - G
990 END
1000 PRINT "BMP280 PRESSURE:   ", H, ".", G, " hPa (", S, " Pa)"
1010 PRINT "========================================"
1020 RETURN
```

### Verified Terminal Output on Real Hardware (`RUN`):
```text
RUN
========================================
CC2541 WEATHER STATION (AHT20 + BMP280)
========================================
AHT20 HUMIDITY:    47.4 %
AHT20 TEMPERATURE: 29.9 C
BMP280 TEMPERATURE:31.38 C
BMP280 PRESSURE:   1009.67 hPa (100967 Pa)
========================================
OK
```

---

## 10. Example 8 [Real Hardware Tested]: PCF8574 / HD44780 16x2 Character LCD (YwRobot LCM1602 I2C V1)

> [!NOTE]
> **Status: Real Hardware Tested** on physical CC2541 hardware connected to a **YwRobot LCM1602 I2C V1** backpack (using PCF8574AT at 7-bit address `63` / `0x3F`) driving a Hitachi HD44780-compatible 16x2 character LCD over dedicated hardware I2C pins (Pin 2 SCL, Pin 3 SDA) via UART COM1.

> [!IMPORTANT]
> **Contrast Adjustment at 3.3V Power Supply:**  
> When operating this LCD backpack on **3.3V $V_{CC}$** (directly from the CC2541 module rail), the LCD contrast threshold is significantly narrower than at 5.0V. Before adjusting the contrast, line 1 will show solid black blocks even after initialization. **You must gently adjust the blue trimmer potentiometer on the back of the backpack with a small screwdriver until the black blocks dissolve and the text characters appear crisp and sharp.**

### Hardware Bit Mapping (PCF8574 -> HD44780):
The YwRobot LCM1602 backpack connects the 8 parallel outputs of the PCF8574 expander as follows:

| PCF8574 Pin | Bit Mask | HD44780 Signal | Function |
| :--- | :--- | :--- | :--- |
| **P0** | `0x01` (`1`) | **RS** | Register Select (`0` = Command, `1` = Character Data) |
| **P1** | `0x02` (`2`) | **RW** | Read / Write (`0` = Write) |
| **P2** | `0x04` (`4`) | **EN** | Enable clock strobe (latches data on falling edge) |
| **P3** | `0x08` (`8`) | **Backlight** | NPN transistor switch (`1` = ON, `0` = OFF) |
| **P4** | `0x10` (`16`) | **DB4** | High-nibble / 4-bit data line 4 |
| **P5** | `0x20` (`32`) | **DB5** | High-nibble / 4-bit data line 5 |
| **P6** | `0x40` (`64`) | **DB6** | High-nibble / 4-bit data line 6 |
| **P7** | `0x80` (`128`) | **DB7** | High-nibble / 4-bit data line 7 |

### 4-Bit Protocol & Subroutine Architecture:
1. **Wake-up & 4-Bit Switch:** The HD44780 powers on in 8-bit mode. BlueBASIC sends three consecutive `0x30` wake-up strobes (`48 | L | 4`, `48 | L`), then switches the controller down to 4-bit mode by strobing `0x20` (`32 | L | 4`, `32 | L`).
2. **Configuration Sequence:**
   - Function Set (`0x28` = 40): 4-bit mode, 2 display lines, 5x8 font.
   - Display Control (`0x08` = 8): Display OFF.
   - Clear Display (`0x01` = 1): Wipes DDRAM (requires ~2ms delay).
   - Entry Mode Set (`0x06` = 6): Auto-increment cursor, no display shift.
   - Display ON (`0x0C` = 12): Display ON, cursor OFF.
3. **Single I2C 6-Byte 3-Phase Clocked Strobe:**
   For any byte $B$ ($H = B \ \& \ 240, U = (B \ll 4) \ \& \ 240, E = L \mid R$):
   The HD44780 requires data and RS to be stable before and after EN is toggled. BlueBASIC executes this via an atomic 6-byte hardware transaction (Data Stable $\rightarrow$ EN High $\rightarrow$ EN Low for each nibble):
   `HWI2C WRITE A, (H | E), (H | E | 4), (H | E), (U | E), (U | E | 4), (U | E)`
4. **Line DDRAM Addresses:**
   - **Line 1 (col 0..15):** Starts at command `128` (`0x80`).
   - **Line 2 (col 0..15):** Starts at command `192` (`0xC0`).

```basic
10 REM ========================================================
20 REM CC2541 HWI2C PCF8574 / HD44780 16x2 LCD DRIVER
30 REM TARGET: YwRobot LCM1602 I2C V1 (PCF8574AT @ ADDR 63 / 0x3F)
40 REM ========================================================
50 HWI2C MASTER 100
60 A = 63
70 L = 8
80 PRINT "INITIALIZING HD44780 16X2 LCD..."
90 GOSUB 500
100 PRINT "DISPLAYING INFORMATION ON LCD..."
110 REM --- LINE 1: BLUEBASIC CC2541 ---
120 B = 128
122 R = 0
125 GOSUB 800
130 DIM M(16)
140 M = 66, 108, 117, 101, 66, 65, 83, 73, 67, 32, 67, 67, 50, 53, 52, 49
150 FOR I = 0 TO 15
160 B = M(I)
162 R = 1
165 GOSUB 800
170 NEXT I
180 REM --- LINE 2: HD44780 I2C OK! ---
190 B = 192
192 R = 0
195 GOSUB 800
200 DIM N(16)
210 N = 72, 68, 52, 52, 55, 56, 48, 32, 73, 50, 67, 32, 79, 75, 33, 32
220 FOR I = 0 TO 15
230 B = N(I)
232 R = 1
235 GOSUB 800
240 NEXT I
250 PRINT "========================================"
260 PRINT "LCD STATUS: TEXT DISPLAYED SUCCESSFULLY!"
270 PRINT "LINE 1: BlueBASIC CC2541"
280 PRINT "LINE 2: HD44780 I2C OK! "
290 PRINT "========================================"
300 RETURN
500 REM === HD44780 4-BIT INITIALIZATION SUBROUTINE ===
505 REM PULL EN LOW AND RS LOW, TURN BACKLIGHT ON
510 HWI2C WRITE A, L
515 FOR J = 1 TO 500
518 NEXT J
520 REM 1. WAKE UP CONTROLLER (3X 0X30 STROBES: LOW-HIGH-LOW)
522 HWI2C WRITE A, (48 | L), (48 | L | 4), (48 | L)
530 FOR J = 1 TO 500
535 NEXT J
540 HWI2C WRITE A, (48 | L), (48 | L | 4), (48 | L)
550 FOR J = 1 TO 100
555 NEXT J
560 HWI2C WRITE A, (48 | L), (48 | L | 4), (48 | L)
570 FOR J = 1 TO 100
575 NEXT J
580 REM 2. SWITCH INTERFACE TO 4-BIT MODE (0X20: LOW-HIGH-LOW)
590 HWI2C WRITE A, (32 | L), (32 | L | 4), (32 | L)
600 FOR J = 1 TO 100
605 NEXT J
610 REM 3. FUNCTION SET: 4-BIT MODE, 2 LINES, 5X8 FONT (0X28 = 40)
620 B = 40
622 R = 0
625 GOSUB 800
630 REM 4. DISPLAY OFF (0X08 = 8)
640 B = 8
642 R = 0
645 GOSUB 800
650 REM 5. CLEAR DISPLAY (0X01 = 1)
660 B = 1
662 R = 0
665 GOSUB 800
670 FOR J = 1 TO 500
675 NEXT J
680 REM 6. ENTRY MODE SET: INCREMENT CURSOR, NO SHIFT (0X06 = 6)
690 B = 6
692 R = 0
695 GOSUB 800
700 REM 7. DISPLAY ON, CURSOR OFF (0X0C = 12)
710 B = 12
712 R = 0
715 GOSUB 800
720 RETURN
800 REM === LCD SEND BYTE B (R=0: COMMAND, R=1: DATA) ===
805 REM 3-PHASE CLOCKED NIBBLES (DATA STABLE -> EN HIGH -> EN LOW)
810 H = B & 240
820 U = (B << 4) & 240
830 E = L | R
840 HWI2C WRITE A, (H | E), (H | E | 4), (H | E), (U | E), (U | E | 4), (U | E)
850 RETURN
```

---

### Example 8B [Real Hardware Tested]: Dynamic Scrolling Banner & Live 0–100 Counter

> [!NOTE]
> **Status: Real Hardware Tested** on CC2541 running BlueBASIC connected to YwRobot LCM1602 I2C V1 over hardware I2C pins.
> - **Line 1:** 32-character circular text banner continuously scrolling across the 16 LCD columns.
> - **Line 2:** Real-time 3-digit formatted counter incrementing from `0` to `100` (`COUNTER:   0/100` to `COUNTER: 100/100`).
> - **UART Terminal:** Telemetry updates printed to the serial monitor every 10 counts.

```basic
10 REM ========================================================
20 REM CC2541 HWI2C PCF8574 / HD44780 LCD SCROLL & COUNTER
30 REM TARGET: YwRobot LCM1602 I2C V1 (PCF8574AT @ ADDR 63 / 0x3F)
40 REM ========================================================
50 HWI2C MASTER 100
60 A = 63
70 L = 8
80 PRINT "INITIALIZING HD44780 16X2 LCD..."
90 GOSUB 400
100 PRINT "STARTING SCROLL & COUNTER DEMO (0 TO 100)..."
110 DIM M(32)
120 M = 66, 76, 85, 69, 66, 65, 83, 73, 67, 32, 67, 67, 50, 53, 52, 49, 32, 72, 87, 73, 50, 67, 32, 76, 67, 68, 32, 68, 69, 77, 79, 32
130 FOR C = 0 TO 100
140 REM --- 1. UPDATE SCROLLING MARQUEE ON LINE 1 ---
150 B = 128
152 R = 0
155 GOSUB 800
160 FOR K = 0 TO 15
170 P = (C + K) % 32
180 B = M(P)
182 R = 1
185 GOSUB 800
190 NEXT K
200 REM --- 2. UPDATE COUNTER ON LINE 2 ---
210 GOSUB 600
220 REM --- 3. PRINT PROGRESS TO UART EVERY 10 TICKS ---
230 IF (C % 10) == 0
240 PRINT "COUNT: ", C, " / 100"
250 END
260 REM --- 4. PACING DELAY (~200 MS) ---
270 FOR J = 1 TO 2000
275 NEXT J
280 NEXT C
290 PRINT "========================================"
300 PRINT "DEMO COMPLETE! COUNT REACHED 100."
310 PRINT "========================================"
320 RETURN
400 REM === HD44780 4-BIT INITIALIZATION SUBROUTINE ===
405 REM PULL EN LOW AND RS LOW, TURN BACKLIGHT ON
410 HWI2C WRITE A, L
415 FOR J = 1 TO 500
418 NEXT J
420 REM 1. WAKE UP CONTROLLER (3X 0X30 STROBES: LOW-HIGH-LOW)
422 HWI2C WRITE A, (48 | L), (48 | L | 4), (48 | L)
430 FOR J = 1 TO 500
435 NEXT J
440 HWI2C WRITE A, (48 | L), (48 | L | 4), (48 | L)
445 FOR J = 1 TO 100
448 NEXT J
450 HWI2C WRITE A, (48 | L), (48 | L | 4), (48 | L)
455 FOR J = 1 TO 100
458 NEXT J
460 REM 2. SWITCH INTERFACE TO 4-BIT MODE (0X20: LOW-HIGH-LOW)
465 HWI2C WRITE A, (32 | L), (32 | L | 4), (32 | L)
470 FOR J = 1 TO 100
475 NEXT J
480 REM 3. FUNCTION SET: 4-BIT MODE, 2 LINES, 5X8 FONT (0X28 = 40)
485 B = 40
490 R = 0
495 GOSUB 800
500 REM 4. DISPLAY OFF (0X08 = 8)
505 B = 8
510 R = 0
515 GOSUB 800
520 REM 5. CLEAR DISPLAY (0X01 = 1)
525 B = 1
530 R = 0
535 GOSUB 800
540 FOR J = 1 TO 500
545 NEXT J
550 REM 6. ENTRY MODE SET: INCREMENT CURSOR, NO SHIFT (0X06 = 6)
555 B = 6
560 R = 0
565 GOSUB 800
570 REM 7. DISPLAY ON, CURSOR OFF (0X0C = 12)
575 B = 12
580 R = 0
585 GOSUB 800
590 RETURN
600 REM === SUBROUTINE: UPDATE LINE 2 (COUNTER: XXX/100) ===
602 B = 192
604 R = 0
606 GOSUB 800
608 REM PRINT 'COUNTER: '
610 R = 1
612 B = 67
614 GOSUB 800
616 B = 79
618 GOSUB 800
620 B = 85
622 GOSUB 800
624 B = 78
626 GOSUB 800
628 B = 84
630 GOSUB 800
632 B = 69
634 GOSUB 800
636 B = 82
638 GOSUB 800
640 B = 58
642 GOSUB 800
644 B = 32
646 GOSUB 800
648 REM HUNDREDS DIGIT
650 IF C >= 100
652 B = 48 + (C / 100)
654 ELSE
656 B = 32
658 END
660 R = 1
662 GOSUB 800
664 REM TENS DIGIT
666 IF C >= 10
668 B = 48 + ((C / 10) % 10)
670 ELSE
672 B = 32
674 END
676 R = 1
678 GOSUB 800
680 REM UNITS DIGIT
682 B = 48 + (C % 10)
684 R = 1
686 GOSUB 800
688 REM PRINT '/100'
690 B = 47
692 R = 1
694 GOSUB 800
696 B = 49
698 GOSUB 800
700 B = 48
702 GOSUB 800
704 B = 48
706 GOSUB 800
710 RETURN
800 REM === LCD SEND BYTE B (R=0: COMMAND, R=1: DATA) ===
805 REM 3-PHASE CLOCKED NIBBLES (DATA STABLE -> EN HIGH -> EN LOW)
810 H = B & 240
820 U = (B << 4) & 240
830 E = L | R
840 HWI2C WRITE A, (H | E), (H | E | 4), (H | E), (U | E), (U | E | 4), (U | E)
850 RETURN
```

### Verified Terminal Output on Real Hardware (`RUN`):
```text
RUN
INITIALIZING HD44780 16X2 LCD...
STARTING SCROLL & COUNTER DEMO (0 TO 100)...
COUNT: 0 / 100
COUNT: 10 / 100
COUNT: 20 / 100
COUNT: 30 / 100
COUNT: 40 / 100
COUNT: 50 / 100
COUNT: 60 / 100
COUNT: 70 / 100
COUNT: 80 / 100
COUNT: 90 / 100
COUNT: 100 / 100
========================================
DEMO COMPLETE! COUNT REACHED 100.
========================================
OK
```

---

## 11. Example 9 [Real Hardware Tested]: Sensirion SHT30 / SHT31 Temperature, Humidity & Status Telemetry

> [!NOTE]
> **Status: Real Hardware Tested** on physical CC2541 hardware with a **Sensirion SHT30 / SHT31** digital temperature and relative humidity sensor connected to dedicated hardware I2C pins (Pin 2 SCL, Pin 3 SDA) via UART COM1.

### Sensor Overview & Features:
- **I2C Addresses:**
  - `ADDR` connected to `GND`: 7-bit `0x44` (`68`), 8-bit wire `0x88` (`136`)
  - `ADDR` connected to `VDD`: 7-bit `0x45` (`69`), 8-bit wire `0x8A` (`138`)
- **Measurement Commands:**
  - High repeatability, clock stretching enabled: `0x2C06` (`44, 6`)
  - High repeatability, clock stretching disabled: `0x2400` (`36, 0`)
- **Status & Control Commands:**
  - Read Status Register: `0xF32D` (`243, 45`)
  - Clear Status Register: `0x3041` (`48, 65`)
  - Built-in Heater ON: `0x306D` (`48, 109`)
  - Built-in Heater OFF: `0x3066` (`48, 102`)
  - Soft Reset: `0x30A2` (`48, 162`)

### 32-Bit Fixed-Point Integer Math Formulas:
Because BlueBASIC v1.4.0 uses 32-bit signed integers, standard floating-point formulas are scaled by 100 to yield 2 exact decimal places without 32-bit overflow:
1. **Temperature in Celsius ($\times 100$):**
   $$T(^\circ\text{C}) \times 100 = \frac{17500 \times S_T}{65535} - 4500$$
   - Intermediate maximum product: $17500 \times 65535 = 1,146,862,500 < 2^{31}-1$ (safe from overflow).
2. **Temperature in Fahrenheit ($\times 100$):**
   $$T(^\circ\text{F}) \times 100 = \frac{31500 \times S_T}{65535} - 4900$$
   - Intermediate maximum product: $31500 \times 65535 = 2,064,352,500 < 2^{31}-1$ (safe from overflow).
3. **Relative Humidity ($\times 100$):**
   $$RH(\%) \times 100 = \frac{10000 \times S_{RH}}{65535}$$
   - Intermediate maximum product: $10000 \times 65535 = 655,350,000 < 2^{31}-1$ (safe from overflow).

### Status Register Bitfields (`0xF32D`):
- **Bit 15 (`V & 32768`):** Alert Pending status (`0` = None, `1` = Alert active)
- **Bit 13 (`V & 8192`):** Heater status (`0` = OFF, `1` = ON)
- **Bit 11 (`V & 2048`):** Humidity tracking alert
- **Bit 10 (`V & 1024`):** Temperature tracking alert
- **Bit 4 (`V & 16`):** System Reset detected (`1` = Power-on reset or soft reset occurred since last clear)
- **Bit 1 (`V & 2`):** Command execution status (`0` = Success, `1` = Error / Unknown command)
- **Bit 0 (`V & 1`):** Write checksum status (`0` = Valid checksum, `1` = Checksum failed)

### Complete Verified BlueBASIC Program (`sht30_demo.bas`):
```basic
10 REM ==============================================
20 REM Sensirion SHT30/SHT31 Temperature & Humidity
30 REM CC2541 Hardware I2C (Pin 2 SCL, Pin 3 SDA)
40 REM Real Hardware Tested on CC2541 BlueBASIC v1.4.0
50 REM ==============================================
60 A = 68
70 DIM S(3)
80 DIM D(6)
90 HWI2C MASTER 100
100 PRINT "=============================================="
110 PRINT "Sensirion SHT30/SHT31 Sensor Initialized"
120 PRINT "I2C 7-bit Addr: 0x44 (68)"
130 PRINT "=============================================="
140 PRINT "--- Initial Status Register ---"
150 GOSUB 1000
160 PRINT "--- Testing Built-in Heater Control ---"
170 PRINT "Turning Heater ON (0x306D)..."
180 HWI2C WRITE A, 48, 109
190 GOSUB 1000
200 PRINT "Turning Heater OFF (0x3066)..."
210 HWI2C WRITE A, 48, 102
220 GOSUB 1000
230 PRINT "Clearing Status Flags (0x3041)..."
240 HWI2C WRITE A, 48, 65
250 GOSUB 1000
260 PRINT "=============================================="
270 PRINT "Starting Temperature & Humidity Readings:"
280 PRINT "=============================================="
290 FOR N = 1 TO 5
300 REM Trigger measurement with clock stretch (0x2C06)
310 HWI2C WRITE A, 44, 6
320 HWI2C READ A, D
330 X = (D(0) << 8) | D(1)
340 Y = (D(3) << 8) | D(4)
350 T = (17500 * X) / 65535 - 4500
360 E = (31500 * X) / 65535 - 4900
370 H = (10000 * Y) / 65535
380 PRINT "--- Sample #", N, " ---"
390 PRINT "Raw Temp    : ", X, " (CRC: ", D(2), ")"
400 I = T / 100
410 F = T % 100
420 IF F < 10
430 PRINT "Temp (C)    : ", I, ".0", F, " deg C"
440 ELSE
450 PRINT "Temp (C)    : ", I, ".", F, " deg C"
460 END
470 I = E / 100
480 F = E % 100
490 IF F < 10
500 PRINT "Temp (F)    : ", I, ".0", F, " deg F"
510 ELSE
520 PRINT "Temp (F)    : ", I, ".", F, " deg F"
530 END
540 PRINT "Raw Humidity: ", Y, " (CRC: ", D(5), ")"
550 I = H / 100
560 F = H % 100
570 IF F < 10
580 PRINT "Humidity    : ", I, ".0", F, " %RH"
590 ELSE
600 PRINT "Humidity    : ", I, ".", F, " %RH"
610 END
620 DELAY 1200
630 NEXT N
640 PRINT "=============================================="
650 PRINT "SHT30/SHT31 Demo Complete!"
660 PRINT "=============================================="
670 RETURN
1000 REM --- Subroutine: Read & Decode Status Register ---
1010 HWI2C WRITE A, 243, 45, READ S
1020 V = (S(0) << 8) | S(1)
1030 PRINT "Status Word : ", V, " (CRC: ", S(2), ")"
1040 IF (V & 8192) <> 0
1050 PRINT "  Heater       : ON"
1060 ELSE
1070 PRINT "  Heater       : OFF"
1080 END
1090 IF (V & 16) <> 0
1100 PRINT "  Reset Flag   : Reset Detected (POR)"
1110 ELSE
1120 PRINT "  Reset Flag   : Cleared"
1130 END
1140 IF (V & 32768) <> 0
1150 PRINT "  Alert Pending: YES"
1160 ELSE
1170 PRINT "  Alert Pending: No"
1180 END
1190 IF (V & 2) <> 0
1200 PRINT "  Command Exec : Failed"
1210 ELSE
1220 PRINT "  Command Exec : Success"
1230 END
1240 IF (V & 1) <> 0
1250 PRINT "  Checksum Stat: Failed"
1260 ELSE
1270 PRINT "  Checksum Stat: OK"
1280 END
1290 RETURN
```

### Verified Terminal Output on Real Hardware (`RUN`):
```text
RUN
==============================================
Sensirion SHT30/SHT31 Sensor Initialized
I2C 7-bit Addr: 0x44 (68)
==============================================
--- Initial Status Register ---
Status Word : 0 (CRC: 129)
  Heater       : OFF
  Reset Flag   : Cleared
  Alert Pending: No
  Command Exec : Success
  Checksum Stat: OK
--- Testing Built-in Heater Control ---
Turning Heater ON (0x306D)...
Status Word : 8192 (CRC: 93)
  Heater       : ON
  Reset Flag   : Cleared
  Alert Pending: No
  Command Exec : Success
  Checksum Stat: OK
Turning Heater OFF (0x3066)...
Status Word : 0 (CRC: 129)
  Heater       : OFF
  Reset Flag   : Cleared
  Alert Pending: No
  Command Exec : Success
  Checksum Stat: OK
Clearing Status Flags (0x3041)...
Status Word : 0 (CRC: 129)
  Heater       : OFF
  Reset Flag   : Cleared
  Alert Pending: No
  Command Exec : Success
  Checksum Stat: OK
==============================================
Starting Temperature & Humidity Readings:
==============================================
--- Sample #1 ---
Raw Temp    : 28528 (CRC: 181)
Temp (C)    : 31.17 deg C
Temp (F)    : 88.12 deg F
Raw Humidity: 32711 (CRC: 243)
Humidity    : 49.91 %RH
--- Sample #2 ---
Raw Temp    : 28523 (CRC: 28)
Temp (C)    : 31.16 deg C
Temp (F)    : 88.09 deg F
Raw Humidity: 32660 (CRC: 222)
Humidity    : 49.83 %RH
--- Sample #3 ---
Raw Temp    : 28507 (CRC: 217)
Temp (C)    : 31.12 deg C
Temp (F)    : 88.02 deg F
Raw Humidity: 32690 (CRC: 254)
Humidity    : 49.88 %RH
--- Sample #4 ---
Raw Temp    : 28512 (CRC: 246)
Temp (C)    : 31.13 deg C
Temp (F)    : 88.04 deg F
Raw Humidity: 32675 (CRC: 140)
Humidity    : 49.85 %RH
--- Sample #5 ---
Raw Temp    : 28507 (CRC: 217)
Temp (C)    : 31.12 deg C
Temp (F)    : 88.02 deg F
Raw Humidity: 32681 (CRC: 87)
Humidity    : 49.86 %RH
==============================================
SHT30/SHT31 Demo Complete!
==============================================
OK
```

---

## 12. Example 10 [Real Hardware Tested]: MPU-6050 (GY-521) 6-Axis Accelerometer, Gyroscope & Temperature Sensor

> [!NOTE]
> **Status: Real Hardware Tested** on physical CC2541 hardware connected to GY-521 (MPU-6050) over dedicated hardware I2C pins (Pin 2 SCL, Pin 3 SDA) via UART COM1.

The **MPU-6050** combines a 3-axis MEMS accelerometer, a 3-axis MEMS gyroscope, and an internal digital temperature sensor on a single silicon die.

### Hardware Configuration & Addressing:
- **Module:** GY-521 with on-board 3.3V LDO regulator and I2C pull-up resistors.
- **AD0 Pin NC (Floating/Grounded):** 7-bit Address = `0x68` (decimal `104`), 8-bit wire address = `0xD0` (decimal `208`).
- **AD0 Pin High (3.3V):** 7-bit Address = `0x69` (decimal `105`), 8-bit wire address = `0xD2` (decimal `210`).

### Key Registers & Operations:
1. **Wake-up from Sleep:** By default at power-on, the MPU-6050 enters sleep mode (`PWR_MGMT_1` bit 6 = 1). Clear sleep and select PLL with X-axis gyro clock:
   `HWI2C WRITE 104, 107, 1`
2. **Digital Low-Pass Filter (DLPF):** Configured at register 26 (`CONFIG`) with value `3` (42 Hz bandwidth) to reduce high-frequency mechanical vibration noise.
3. **Sample Rate Divider:** Configured at register 25 (`SMPLRT_DIV`) with value `7` (125 Hz).
4. **Ranges & Calculations:**
   - **Accelerometer:** ±2g (Sensitivity: 16,384 LSB/g) -> calculated to milli-g ($1\text{g} = 1000\text{ mg}$) via `(RAW * 1000) / 16384`.
   - **Gyroscope:** ±250 °/s (Sensitivity: 131 LSB/(°/s)) -> calculated to deg/s via `RAW / 131`.
   - **Temperature:** $T(^\circ\text{C}) = \frac{\text{RAW}}{340} + 36.53$ -> scaled to 2 decimal places using fixed-point math: `E = ((RAW * 100) / 340) + 3653`.
5. **Single Burst 14-Byte Read:** Reads all 14 continuous registers (`0x3B` to `0x48`) in one I2C transaction into `DIM D(14)`.

```basic
10 REM ========================================================
20 REM CC2541 HARDWARE I2C (HWI2C) MPU-6050 6-AXIS IMU + TEMP
30 REM GY-521 MODULE (AD0 = NC, 7-BIT ADDR 104 / 0x68)
40 REM ========================================================
50 HWI2C MASTER 100
60 PRINT "========================================"
70 PRINT "MPU-6050 (GY-521) REAL-TIME SENSOR"
80 PRINT "========================================"
90 REM 1. WAKE UP MPU-6050 (SET CLK TO PLL X-GYRO)
100 HWI2C WRITE 104, 107, 1
110 FOR I = 1 TO 500
115 NEXT I
120 REM 2. CONFIGURE DLPF (42 HZ LOW-PASS FILTER)
130 HWI2C WRITE 104, 26, 3
140 REM 3. CONFIGURE SAMPLE RATE DIVIDER (125 HZ)
150 HWI2C WRITE 104, 25, 7
160 REM 4. CONFIGURE GYRO (+-250 DEG/S) & ACCEL (+-2G)
170 HWI2C WRITE 104, 27, 0
180 HWI2C WRITE 104, 28, 0
190 REM 5. VERIFY WHO_AM_I REGISTER (REG 117 = 0x75)
200 DIM W(1)
210 HWI2C WRITE 104, 117, READ W
220 PRINT "CHIP WHO_AM_I ID: ", W(0)
230 IF W(0) != 104
240   PRINT "WARNING: UNEXPECTED CHIP ID (EXPECTED 104)!"
250 END
260 PRINT "STARTING REAL-TIME SENSOR STREAM..."
270 PRINT "========================================"
280 DIM D(14)
290 REM ------------------------------------------------
300 REM MAIN ACQUISITION LOOP
310 REM ------------------------------------------------
320 HWI2C WRITE 104, 59, READ D
330 REM CONVERT ACCELEROMETER (REG 59..64)
340 A = (D(0) << 8) | D(1)
350 IF D(0) & 128
360   A = A - 65536
370 END
380 B = (D(2) << 8) | D(3)
390 IF D(2) & 128
400   B = B - 65536
410 END
420 C = (D(4) << 8) | D(5)
430 IF D(4) & 128
440   C = C - 65536
450 END
460 REM CONVERT TEMPERATURE (REG 65..66)
470 T = (D(6) << 8) | D(7)
480 IF D(6) & 128
490   T = T - 65536
500 END
510 E = ((T * 100) / 340) + 3653
520 U = E / 100
530 V = E % 100
540 IF V < 0
550   V = 0 - V
560 END
570 REM CONVERT GYROSCOPE (REG 67..72)
580 X = (D(8) << 8) | D(9)
590 IF D(8) & 128
600   X = X - 65536
610 END
620 Y = (D(10) << 8) | D(11)
630 IF D(10) & 128
640   Y = Y - 65536
650 END
660 Z = (D(12) << 8) | D(13)
670 IF D(12) & 128
680   Z = Z - 65536
690 END
700 REM CALCULATE MILLI-G (mg) AND DEG/S
710 J = (A * 1000) / 16384
720 K = (B * 1000) / 16384
730 L = (C * 1000) / 16384
740 P = X / 131
750 Q = Y / 131
760 R = Z / 131
770 REM DISPLAY SENSOR TELEMETRY
780 PRINT "ACCEL (mg):   X=", J, "  Y=", K, "  Z=", L
790 PRINT "GYRO (deg/s): X=", P, "  Y=", Q, "  Z=", R
800 PRINT "TEMP:         ", U, ".", V, " C"
810 PRINT "----------------------------------------"
820 FOR I = 1 TO 4000
830 NEXT I
840 GOTO 320
```

### Verified Terminal Output on Real Hardware (`RUN`):
```text
RUN
========================================
MPU-6050 (GY-521) REAL-TIME SENSOR
========================================
CHIP WHO_AM_I ID: 104
STARTING REAL-TIME SENSOR STREAM...
========================================
ACCEL (mg):   X=589  Y=825  Z=88
GYRO (deg/s): X=0  Y=-1  Z=0
TEMP:         31.50 C
----------------------------------------
ACCEL (mg):   X=589  Y=823  Z=85
GYRO (deg/s): X=0  Y=-1  Z=0
TEMP:         31.49 C
----------------------------------------
ACCEL (mg):   X=591  Y=825  Z=88
GYRO (deg/s): X=0  Y=-1  Z=0
TEMP:         31.49 C
----------------------------------------
ACCEL (mg):   X=591  Y=827  Z=88
GYRO (deg/s): X=0  Y=-1  Z=0
TEMP:         31.51 C
----------------------------------------
ACCEL (mg):   X=590  Y=826  Z=80
GYRO (deg/s): X=0  Y=-1  Z=0
TEMP:         31.50 C
### In-Place Status Bar Mode (No LineFeed / Zero Scrolling):
For high-speed real-time monitoring where scrolling text is distracting or unreadable, this variant emits data terminated **strictly with Carriage Return (`\r` = ASCII 13) without LineFeed (`\n` = ASCII 10)** via `WRITE #SERIAL`. The terminal cursor returns to column 0 after each measurement, overwriting the exact same line in place:

```basic
10 REM ========================================================
20 REM CC2541 HARDWARE I2C (HWI2C) MPU-6050 LIVE STATUS BAR
30 REM NO-LINEFEED (CR ONLY) IN-PLACE UPDATING STREAM
40 REM ========================================================
50 HWI2C MASTER 100
60 HWI2C WRITE 104, 107, 1
70 FOR I = 1 TO 300
75 NEXT I
80 HWI2C WRITE 104, 26, 3
90 HWI2C WRITE 104, 25, 7
100 HWI2C WRITE 104, 27, 0
110 HWI2C WRITE 104, 28, 0
120 DIM D(14)
130 PRINT "================================================================================"
140 PRINT "MPU-6050 LIVE STATUS BAR (UPDATING IN PLACE - NO LINEFEED)"
150 PRINT "================================================================================"
200 REM --- ACQUIRE 14-BYTE SENSOR BURST ---
210 HWI2C WRITE 104, 59, READ D
220 A = (D(0) << 8) | D(1)
230 IF D(0) & 128
232   A = A - 65536
234 END
240 B = (D(2) << 8) | D(3)
250 IF D(2) & 128
252   B = B - 65536
254 END
260 C = (D(4) << 8) | D(5)
270 IF D(4) & 128
272   C = C - 65536
274 END
280 T = (D(6) << 8) | D(7)
290 IF D(6) & 128
292   T = T - 65536
294 END
300 E = ((T * 100) / 340) + 3653
310 U = E / 100
320 V = E % 100
330 IF V < 0
332   V = 0 - V
334 END
340 X = (D(8) << 8) | D(9)
350 IF D(8) & 128
352   X = X - 65536
354 END
360 Y = (D(10) << 8) | D(11)
370 IF D(10) & 128
372   Y = Y - 65536
374 END
380 Z = (D(12) << 8) | D(13)
390 IF D(12) & 128
392   Z = Z - 65536
394 END
400 J = (A * 1000) / 16384
410 K = (B * 1000) / 16384
420 L = (C * 1000) / 16384
430 P = X / 131
440 Q = Y / 131
450 R = Z / 131
500 REM --- EMIT STATUS LINE (CARRIAGE RETURN ONLY, NO LINEFEED) ---
510 WRITE #SERIAL, 65, 91, 109, 103, 93, 58, 88, 61
520 N = J
530 GOSUB 850
540 WRITE #SERIAL, 32, 89, 61
550 N = K
560 GOSUB 850
570 WRITE #SERIAL, 32, 90, 61
580 N = L
590 GOSUB 850
600 WRITE #SERIAL, 32, 124, 32, 71, 91, 100, 47, 115, 93, 58, 88, 61
610 N = P
620 GOSUB 850
630 WRITE #SERIAL, 32, 89, 61
640 N = Q
650 GOSUB 850
660 WRITE #SERIAL, 32, 90, 61
670 N = R
680 GOSUB 850
690 WRITE #SERIAL, 32, 124, 32, 84, 58, 32
700 WRITE #SERIAL, 48 + (U / 10), 48 + (U % 10), 46
710 WRITE #SERIAL, 48 + (V / 10), 48 + (V % 10), 32, 67, 32, 32
720 WRITE #SERIAL, 13
730 DELAY 200
740 GOTO 200
850 REM --- 5-CHAR FIXED-WIDTH RIGHT-ALIGNED INTEGER IN N ---
860 S = 32
870 IF N < 0
872   S = 45
874   N = 0 - N
876 END
880 IF N < 10
882   WRITE #SERIAL, 32, 32, 32, S, 48 + N
884   RETURN
886 END
890 IF N < 100
892   WRITE #SERIAL, 32, 32, S, 48 + (N / 10), 48 + (N % 10)
894   RETURN
896 END
900 IF N < 1000
902   WRITE #SERIAL, 32, S, 48 + (N / 100), 48 + ((N / 10) % 10), 48 + (N % 10)
904   RETURN
906 END
910 IF N < 10000
912   WRITE #SERIAL, S, 48 + (N / 1000), 48 + ((N / 100) % 10), 48 + ((N / 10) % 10), 48 + (N % 10)
914   RETURN
916 END
920 WRITE #SERIAL, 48 + (N / 10000), 48 + ((N / 1000) % 10), 48 + ((N / 100) % 10), 48 + ((N / 10) % 10), 48 + (N % 10)
930 RETURN
```

#### Terminal Live Display (Pinned Single Line, Overwriting In-Place):
```text
================================================================================
MPU-6050 LIVE STATUS BAR (UPDATING IN PLACE - NO LINEFEED)
================================================================================
A[mg]:X=  620 Y=  802 Z=   92 | G[d/s]:X=    0 Y=   -1 Z=    0 | T: 31.39 C  
```

---

## 13. Common I2C Module Address Reference

BlueBASIC supports both 7-bit unshifted addresses and 8-bit wire addresses ($7\text{-bit} \times 2$):

| Device | Description | 7-Bit Hex | 7-Bit Dec | 8-Bit Wire Hex | 8-Bit Decimal |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **SHT30 / SHT31** | High-Accuracy Temp & Humidity | `0x44` / `0x45` | **`68`** / **`69`** | `0x88` / `0x8A` | **`136`** / **`138`** |
| **AHT20** | Humidity & Temperature | `0x38` | **`56`** | `0x70` | **`112`** |
| **BMP280 / BME280** | Pressure / Temp / Humidity | `0x76` / `0x77` | 118 / 119 | `0xEC` / `0xEE` | **`236`** / **`238`** |
| **DS3231 / DS1307** | High-Precision RTC | `0x68` | 104 | `0xD0` | **`208`** |
| **24C32 EEPROM** | Default (A0,A1,A2 Open) | `0x57` | 87 | `0xAE` | **`174`** |
| **24C32 EEPROM** | All Address Pins GND | `0x50` | 80 | `0xA0` | **`160`** |
| **MPU-6050** | 6-Axis Accelerometer / Gyro | `0x68` / `0x69` | 104 / 105 | `0xD0` / `0xD2` | **`208`** / **`210`** |
| **SSD1306** | 0.96" Monochrome OLED | `0x3C` / `0x3D` | 60 / 61 | `0x78` / `0x7A` | **`120`** / **`122`** |
| **PCF8574 (LCD Backpack)** | Default (A0,A1,A2 Open) | `0x27` | **`39`** | `0x4E` | **`78`** |
| **PCF8574AT (YwRobot LCD)** | Default (A0,A1,A2 Open) | `0x3F` | **`63`** | `0x7E` | **`126`** |
| **PCF8574 / PCF8574A** | All Address Pins GND | `0x20` / `0x38` | 32 / 56 | `0x40` / `0x70` | 64 / 112 |
| **LM75A / TMP102** | Digital Temperature | `0x48` | 72 | `0x90` | **`144`** |

---

## 13. Template for Adding New Hardware I2C Sensors

Use this boilerplate to interface with any new I2C sensor:

```basic
10 REM ========================================================
20 REM NEW I2C SENSOR DRIVER TEMPLATE
30 REM ========================================================
40 HWI2C MASTER 100
50 REM STEP 1: INITIALIZE / CONFIGURE SENSOR
60 REM HWI2C WRITE <DEVICE_ADDR>, <CONFIG_REG>, <CONFIG_VALUE>
70 REM STEP 2: READ SENSOR DATA REGISTERS
80 DIM D(6)
90 REM HWI2C WRITE <DEVICE_ADDR>, <START_REG>, READ D
100 REM STEP 3: PROCESS DATA BYTES
110 REM X = (D(0) << 8) | D(1)
120 REM PRINT "VALUE: ", X
130 RETURN
```


