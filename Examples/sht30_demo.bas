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
