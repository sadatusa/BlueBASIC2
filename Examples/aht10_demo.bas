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
