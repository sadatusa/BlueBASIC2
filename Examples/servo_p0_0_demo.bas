10 REM ========================================================
20 REM Real Servo Demonstration Script for Pin P0_0
30 REM ========================================================
40 REM Attach Servo on P0_0 at 90 deg Center
50 SERVO ATTACH P0(0), 90
60 PRINT "Servo attached on P0_0. Current angle: "; SERVO(0)
70 DELAY 1000
80 REM --------------------------------------------------------
90 REM Smooth Sweep from Center to 0 deg
100 PRINT "Moving to 0 deg..."
110 SERVO P0(0) = 0, 60
120 DELAY 2000
130 PRINT "Reached: "; SERVO(0); " deg"
140 REM --------------------------------------------------------
150 REM Sweep from 0 to 180 deg at 45 deg/sec
160 PRINT "Sweeping from 0 to 180 deg at 45 deg/sec..."
170 SERVO P0(0) = 180, 45
180 FOR I = 1 TO 9
190   PRINT "  T+"; I * 500; "ms -> Angle: "; SERVO(0); " deg"
200   DELAY 500
210 NEXT I
220 REM --------------------------------------------------------
230 REM Return to Center (90 deg) at 90 deg/sec
240 PRINT "Returning to Center (90 deg)..."
250 SERVO P0(0) = 90, 90
260 DELAY 1500
270 PRINT "Final Position: "; SERVO(0); " deg"
280 REM Detach Servo
290 SERVO DETACH P0(0)
300 PRINT "Servo detached. Finished!"
