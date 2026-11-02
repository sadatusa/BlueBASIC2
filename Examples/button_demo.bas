10 REM ========================================================
15 REM STATUS: REAL HARDWARE TESTED (CC2541 HM-10)
20 REM BUTTON INTERRUPT & LED DEMO (P1_3, P0_5, P1_2, P0_7)
30 REM ========================================================
40 PINMODE P1(2) OUTPUT
50 PINMODE P0(7) OUTPUT
60 P1(2) = 0
70 P0(7) = 0
80 PINMODE P1(3) INPUT PULLUP
90 PINMODE P0(5) INPUT PULLUP
100 A = 0
110 B = 0
120 BUTTON ATTACH P1(3), FALLING, 50 GOSUB 500
130 BUTTON ATTACH P0(5), FALLING, 50 GOSUB 600
140 PRINT "========================================================"
150 PRINT "BUTTON INTERRUPT SYSTEM ARMED & ACTIVE!"
160 PRINT "  Button 1 (P1_3) -> Blinks Blue LED (P1_2)"
170 PRINT "  Button 2 (P0_5) -> Blinks Green LED (P0_7)"
180 PRINT "Waiting for button presses (Monitoring active)..."
190 PRINT "========================================================"
200 FOR I = 1 TO 15
210   DELAY 1000
220 NEXT I
230 PRINT "========================================================"
240 PRINT "MONITOR SUMMARY: Button 1: "; A; " | Button 2: "; B
250 PRINT "Background button interrupts remain active in memory."
260 PRINT "========================================================"
270 END
500 REM --- BUTTON 1 INTERRUPT HANDLER (P1_3) ---
510 A = A + 1
520 PRINT ">>> [EVENT] BUTTON 1 (P1_3) PRESSED! (TOTAL: "; A; ")"
530 P1(2) = 1
540 DELAY 200
550 P1(2) = 0
560 RETURN
600 REM --- BUTTON 2 INTERRUPT HANDLER (P0_5) ---
610 B = B + 1
620 PRINT ">>> [EVENT] BUTTON 2 (P0_5) PRESSED! (TOTAL: "; B; ")"
630 P0(7) = 1
640 DELAY 200
650 P0(7) = 0
660 RETURN
