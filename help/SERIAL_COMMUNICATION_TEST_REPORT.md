# BlueBASIC Serial Communication & Program Execution Report

**Target Platform:** Texas Instruments CC2541 (HM-10 BLE Module)  
**Hardware Configuration:** UART1 Alternative 2 (`P1_6` TX / `P1_7` RX), Internal 32 kHz RC Oscillator (Calibrated)  
**Host Interface:** COM1 @ 9600 Baud, 8-N-1  
**Programmer Interface:** Arduino Uno CCLoader on COM2  
**Test Suite Date:** October 23, 2026  
**Final Test Result:** **20 / 20 PASS (100% Success Rate)**  

---

## 1. Executive Summary

This report documents comprehensive serial communication and interpreter testing of **BlueBASIC** running on a **Texas Instruments CC2541 HM-10 module**. 

A complete suite of **20 distinct BlueBASIC programs**, each containing **between 10 and 19 lines of code**, was developed, uploaded over serial UART, verified in non-volatile flash storage (`LIST`), and executed (`RUN`). 

Every program executed autonomously without human intervention and achieved **100% pass rate (20/20)**.

### Test Suite Scorecard

| ID | Program Name | Category | Lines | Upload Time | Exec Time | Result |
|:--:|:---|:---|:--:|:--:|:--:|:--:|
| **P01** | Arithmetic and Bitwise Operators | Core Math & Logic | 15 | 6.2 s | 248 ms | **PASS** |
| **P02** | Fibonacci Sequence Generator | Iteration & Sequences | 14 | 5.8 s | 261 ms | **PASS** |
| **P03** | Prime Number Checker | Number Theory & Loops | 17 | 6.8 s | 432 ms | **PASS** |
| **P04** | Factorial & Permutations | Combinatorics & Math | 15 | 5.9 s | 355 ms | **PASS** |
| **P05** | Formatted Output & Table | Terminal Formatting | 16 | 7.7 s | 465 ms | **PASS** |
| **P06** | Number Classifier | Conditional Logic | 17 | 6.5 s | 328 ms | **PASS** |
| **P07** | Subroutine Modular Math | GOSUB / RETURN Stack | 14 | 5.6 s | 277 ms | **PASS** |
| **P08** | Array Allocation & Summation | DIM & Array Indexing | 13 | 5.2 s | 283 ms | **PASS** |
| **P09** | Bitwise Masking & Binary Bits | Low-Level Bit Shifting | 14 | 5.4 s | 296 ms | **PASS** |
| **P10** | Linear Search Algorithm | Array Traversal & Search | 17 | 5.8 s | 231 ms | **PASS** |
| **P11** | Bubble Sort on Array | Sorting Algorithms | 17 | 6.5 s | 215 ms | **PASS** |
| **P12** | Temperature Conversion Table | Math Calculations | 12 | 6.1 s | 421 ms | **PASS** |
| **P13** | Euclidean GCD Algorithm | Classic Algorithms | 15 | 5.3 s | 266 ms | **PASS** |
| **P14** | Collatz 3N+1 Sequence | Branching & Loops | 19 | 6.7 s | 374 ms | **PASS** |
| **P15** | Compound Growth Model | Financial Modeling | 12 | 5.7 s | 466 ms | **PASS** |
| **P16** | Random Sampling & Statistics | RND & Accumulation | 18 | 6.4 s | 249 ms | **PASS** |
| **P17** | System Diagnostics & Benchmark | Hardware Timers & ADC | 16 | 6.4 s | 358 ms | **PASS** |
| **P18** | Countdown Engine with DELAY | Timers & Micro-delays | 10 | 4.9 s | 829 ms | **PASS** |
| **P19** | Pythagorean Triples Discovery | Multi-Nested Loops | 15 | 7.0 s | 2004 ms | **PASS** |
| **P20** | Digital Logic Truth Tables | Boolean Gates | 14 | 6.5 s | 377 ms | **PASS** |

---

## 2. Hardware Architecture & Firmware Configuration

### 2.1 Hardware Profile
- **Target Chip:** Texas Instruments CC2541F256 (256 KB Flash, 8 KB SRAM, 8051 Core).
- **Module Form Factor:** HM-10 breakout without external 32.768 kHz low-frequency watch crystal.
- **Clock System:**
  - High-Speed: 32 MHz crystal oscillator (XOSC).
  - Low-Speed (Sleep/Timing): Internal 32.768 kHz RC Oscillator (RCOSC) with continuous hardware calibration enabled (`SLEEPCMD &= ~0x80`).
- **UART Port Mapping:** UART1 Alternative 2 configuration:
  - **TX:** Pin `P1_6` (Peripheral Output)
  - **RX:** Pin `P1_7` (Peripheral Input / Wake Interrupt)
  - **Baud Rate:** 9600 Baud, 8 Data Bits, No Parity, 1 Stop Bit (8-N-1).

### 2.2 Root Cause Analysis of Serial Instability & Resolution
During early testing, sporadic character drops (e.g. `PRIN` instead of `PRINT`, or dropped parentheses `VI)`) were observed on UART1. A deep architectural trace through the firmware and TI BLE stack revealed the following critical factors:

1. **Clock Division During MCU Halt:**
   In [BlueBasic.c](file:///..\BlueBASIC\BLE-CC254x-1.4.0/Projects/ble/BlueBasic/Source/BlueBasic.c), line 284 originally contained:
   ```c
   HCI_EXT_ClkDivOnHaltCmd(HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT);
   ```
   This TI BLE extension instructed the chip to down-clock the 32 MHz system clock whenever the 8051 MCU halted during background BLE radio activity. Because the CC2541 hardware UART baud rate generator is derived directly from the system clock, dividing the clock down shifted the baud rate clock, causing framing errors and corrupted bytes on the serial RX FIFO.
   - **Fix:** Switched to `HCI_EXT_ClkDivOnHaltCmd(HCI_EXT_DISABLE_CLK_DIVIDE_ON_HALT);`. The MCU clock now remains steady at 32 MHz constantly, eliminating all baud rate drift.

2. **BLE Advertising Collision:**
   The BLE peripheral default advertising interval was relaxed from 500 ms to 1000 ms (`#define DEFAULT_ADVERTISING_INTERVAL 1600`), cutting radio interrupt collisions by 50% while maintaining fast BLE connectivity.

3. **Interpreter Flashstore Writing Synchronization:**
   Upon receiving each line's terminating newline (`\r\n`), BlueBASIC commits the tokenized line into flash memory via DMA (`HalFlashWrite`). During this flash write window (~15–30 ms), the interpreter sets its input state to `MODE_RUNNING`, dropping incoming characters.
   - **Fix:** Implemented echo-synchronized transmission in the serial test runner. The host waits for each line's newline character to echo back from CC2541, followed by a brief 70 ms settle period for flash page commitment before sending subsequent commands.

---

## 3. Complete Catalog of 20 Test Programs & Results

Below is the complete documentation for all 20 tested programs, including source code, flash store verification (`LIST`), and execution terminal output (`RUN`).

---

### Test 01: Arithmetic and Bitwise Operators
- **Lines:** 15  
- **Purpose:** Validates 32-bit integer arithmetic, operator precedence, modulus, bitwise shifts (`<<`, `>>`), bitwise AND (`&`), OR (`|`), and XOR (`^`).

#### Source Code:
```basic
10 REM P1: ARITHMETIC AND OPERATORS
20 A = 100
30 B = 25
40 C = A + B * 2
50 D = (A - B) / 5
60 E = A % 7
70 F = B << 2
80 G = B >> 1
90 H = A & 15
100 I = A | 15
110 J = A ^ 15
120 PRINT "C=", C, " D=", D
130 PRINT "E=", E, " F=", F, " G=", G
140 PRINT "H=", H, " I=", I, " J=", J
150 PRINT "TEST 1 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
C=150 D=15
E=2 F=100 G=12
H=4 I=111 J=107
TEST 1 COMPLETE
OK
```
- **Execution Time:** 248 ms  
- **Result:** **PASS**

---

### Test 02: Fibonacci Sequence Generator
- **Lines:** 14  
- **Purpose:** Generates the first 10 terms of the Fibonacci sequence using a `FOR...NEXT` loop and three scalar variables.

#### Source Code:
```basic
10 REM P2: FIBONACCI GENERATOR
20 A = 0
30 B = 1
40 PRINT "FIBONACCI SERIES:"
50 PRINT A
60 PRINT B
70 FOR I = 3 TO 10
80   C = A + B
90   PRINT C
100  A = B
110  B = C
120 NEXT I
130 PRINT "TOTAL TERMS: 10"
140 PRINT "TEST 2 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
FIBONACCI SERIES:
0
1
1
2
3
5
8
13
21
34
TOTAL TERMS: 10
TEST 2 COMPLETE
OK
```
- **Execution Time:** 261 ms  
- **Result:** **PASS**

---

### Test 03: Prime Number Checker
- **Lines:** 17  
- **Purpose:** Tests nested `FOR` loops and conditional division to classify numbers from 2 to 15 as prime or composite.

#### Source Code:
```basic
10 REM P3: PRIME NUMBER CHECKER
20 FOR N = 2 TO 15
30   P = 1
40   IF N > 2
50     FOR D = 2 TO N - 1
60       IF N % D == 0
70         P = 0
80       END
90     NEXT D
100  END
110  IF P == 1
120    PRINT N, " IS PRIME"
130  ELSE
140    PRINT N, " IS COMPOSITE"
150  END
160 NEXT N
170 PRINT "TEST 3 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
2 IS PRIME
3 IS PRIME
4 IS COMPOSITE
5 IS PRIME
6 IS COMPOSITE
7 IS PRIME
8 IS COMPOSITE
9 IS COMPOSITE
10 IS COMPOSITE
11 IS PRIME
12 IS COMPOSITE
13 IS PRIME
14 IS COMPOSITE
15 IS COMPOSITE
TEST 3 COMPLETE
OK
```
- **Execution Time:** 432 ms  
- **Result:** **PASS**

---

### Test 04: Factorial & Permutations
- **Lines:** 15  
- **Purpose:** Computes cumulative factorials $1!$ through $7!$, permutation calculation $P(7,4) = 7! / (7-4)!$, and sum of squares $\sum_{i=1}^7 i^2$.

#### Source Code:
```basic
10 REM P4: FACTORIAL CALCULATOR
20 F = 1
30 PRINT "FACTORIAL TABLE:"
40 FOR N = 1 TO 7
50   F = F * N
60   PRINT N, "! = ", F
70 NEXT N
80 P = F / 6
90 PRINT "PERMUTATION P(7,4) = ", P
100 S = 0
110 FOR I = 1 TO 7
120  S = S + I * I
130 NEXT I
140 PRINT "SUM OF SQUARES: ", S
150 PRINT "TEST 4 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
FACTORIAL TABLE:
1! = 1
2! = 2
3! = 6
4! = 24
5! = 120
6! = 720
7! = 5040
PERMUTATION P(7,4) = 840
SUM OF SQUARES: 140
TEST 4 COMPLETE
OK
```
- **Execution Time:** 355 ms  
- **Result:** **PASS**

---

### Test 05: Formatted Output & Table
- **Lines:** 16  
- **Purpose:** Tests formatted tabular output and cumulative square summation.

#### Source Code:
```basic
10 REM P5: TABLE AND FORMATTING
20 PRINT "================================"
30 PRINT "DEC   HEX   SQR   CUBE"
40 PRINT "--------------------------------"
50 FOR X = 1 TO 6
60   S = X * X
70   C = S * X
80   PRINT X, "    ", S, "    ", C
90 NEXT X
100 PRINT "================================"
110 T = 0
120 FOR X = 1 TO 6
130  T = T + X * X
140 NEXT X
150 PRINT "TOTAL SQR SUM = ", T
160 PRINT "TEST 5 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
================================
DEC   HEX   SQR   CUBE
--------------------------------
1    1    1
2    4    8
3    9    27
4    16    64
5    25    125
6    36    216
================================
TOTAL SQR SUM = 91
TEST 5 COMPLETE
OK
```
- **Execution Time:** 465 ms  
- **Result:** **PASS**

---

### Test 06: Number Classifier
- **Lines:** 17  
- **Purpose:** Tests multi-branch conditional structures across positive, negative, zero, odd, and even numbers.

#### Source Code:
```basic
10 REM P6: NUMBER CLASSIFIER
20 FOR V = -2 TO 4
30   IF V < 0
40     PRINT V, " IS NEGATIVE"
50   END
60   IF V == 0
70     PRINT V, " IS ZERO"
80   END
90   IF V > 0
100    IF V % 2 == 0
110      PRINT V, " IS POSITIVE EVEN"
120    ELSE
130      PRINT V, " IS POSITIVE ODD"
140    END
150  END
160 NEXT V
170 PRINT "TEST 6 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
-2 IS NEGATIVE
-1 IS NEGATIVE
0 IS ZERO
1 IS POSITIVE ODD
2 IS POSITIVE EVEN
3 IS POSITIVE ODD
4 IS POSITIVE EVEN
TEST 6 COMPLETE
OK
```
- **Execution Time:** 328 ms  
- **Result:** **PASS**

---

### Test 07: Subroutine Modular Math (GOSUB/RETURN)
- **Lines:** 14  
- **Purpose:** Tests execution stack frames, function modularity with `GOSUB`, local parameter passing via global registers, and clean `RETURN`.

#### Source Code:
```basic
10 REM P7: GOSUB AND RETURN
20 PRINT "STARTING MODULAR MATH"
30 X = 5
40 GOSUB 100
50 PRINT "5 SQUARED = ", R
60 X = 9
70 GOSUB 100
80 PRINT "9 SQUARED = ", R
90 GOTO 150
100 REM SUBROUTINE SQUARE
110 R = X * X
120 RETURN
150 PRINT "SUBROUTINE TEST PASSED"
160 PRINT "TEST 7 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
STARTING MODULAR MATH
5 SQUARED = 25
9 SQUARED = 81
SUBROUTINE TEST PASSED
TEST 7 COMPLETE
OK
```
- **Execution Time:** 277 ms  
- **Result:** **PASS**

---

### Test 08: Array Allocation and Summation (DIM)
- **Lines:** 13  
- **Purpose:** Validates 1D array allocation with `DIM`, indexed storage, array access in loops, and accumulation.

#### Source Code:
```basic
10 REM P8: ARRAY MANIPULATION
20 DIM A(5)
30 FOR I = 0 TO 4
40   A(I) = (I + 1) * 10
50 NEXT I
60 PRINT "ARRAY POPULATED:"
70 S = 0
80 FOR I = 0 TO 4
90   PRINT "A(", I, ") = ", A(I)
100  S = S + A(I)
110 NEXT I
120 PRINT "ARRAY SUM = ", S
130 PRINT "TEST 8 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
ARRAY POPULATED:
A(0) = 10
A(1) = 20
A(2) = 30
A(3) = 40
A(4) = 50
ARRAY SUM = 150
TEST 8 COMPLETE
OK
```
- **Execution Time:** 283 ms  
- **Result:** **PASS**

---

### Test 09: Bitwise Masking & Binary Bits
- **Lines:** 14  
- **Purpose:** Extracts individual binary bits from MSB (bit 7) down to LSB (bit 0) using dynamic shift masks and `STEP -1`.

#### Source Code:
```basic
10 REM P9: BINARY BIT EXTRACTION
20 N = 181
30 PRINT "NUMBER: ", N
40 PRINT "BINARY BITS (7 TO 0):"
50 FOR B = 7 TO 0 STEP -1
60   M = 1 << B
70   IF N & M
80     PRINT "1"
90   ELSE
100    PRINT "0"
110  END
120 NEXT B
130 PRINT "BIT EXTRACTION DONE"
140 PRINT "TEST 9 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
NUMBER: 181
BINARY BITS (7 TO 0):
1
0
1
1
0
1
0
1
BIT EXTRACTION DONE
TEST 9 COMPLETE
OK
```
- **Execution Time:** 296 ms  
- **Result:** **PASS**

---

### Test 10: Linear Search Algorithm
- **Lines:** 17  
- **Purpose:** Allocates an array of unsorted elements, searches for target value `77`, and determines its exact zero-based index.

#### Source Code:
```basic
10 REM P10: LINEAR SEARCH
20 DIM V(6)
30 V(0) = 45
40 V(1) = 12
50 V(2) = 89
60 V(3) = 33
70 V(4) = 77
80 V(5) = 21
90 T = 77
100 F = -1
110 FOR I = 0 TO 5
120  IF V(I) == T
130    F = I
140  END
150 NEXT I
160 PRINT "TARGET: ", T, " FOUND AT INDEX: ", F
170 PRINT "TEST 10 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
TARGET: 77 FOUND AT INDEX: 4
TEST 10 COMPLETE
OK
```
- **Execution Time:** 231 ms  
- **Result:** **PASS**

---

### Test 11: Bubble Sort on Array
- **Lines:** 17  
- **Purpose:** Tests nested `FOR` loops and in-place element swapping to sort 4 elements in ascending order.

#### Source Code:
```basic
10 REM P11: BUBBLE SORT
20 DIM S(4)
30 S(0) = 50
40 S(1) = 20
50 S(2) = 40
60 S(3) = 10
70 FOR I = 0 TO 2
80   FOR J = 0 TO 2 - I
90     IF S(J) > S(J + 1)
100      T = S(J)
110      S(J) = S(J + 1)
120      S(J + 1) = T
130    END
140  NEXT J
150 NEXT I
160 PRINT "SORTED: ", S(0), S(1), S(2), S(3)
170 PRINT "TEST 11 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
SORTED: 10 20 40 50
TEST 11 COMPLETE
OK
```
- **Execution Time:** 215 ms  
- **Result:** **PASS**

---

### Test 12: Temperature Conversion Table
- **Lines:** 12  
- **Purpose:** Computes Celsius to Fahrenheit and Kelvin mappings, including water boiling temperature.

#### Source Code:
```basic
10 REM P12: TEMPERATURE TABLE
20 PRINT "CELSIUS   FAHRENHEIT   KELVIN"
30 FOR C = 0 TO 40 STEP 10
40   F = (C * 9 / 5) + 32
50   K = C + 273
60   PRINT C, " C    = ", F, " F    = ", K, " K"
70 NEXT C
80 C = 100
90 F = (C * 9 / 5) + 32
100 K = C + 273
110 PRINT "BOILING: ", C, "C = ", F, "F = ", K, "K"
120 PRINT "TEST 12 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
CELSIUS   FAHRENHEIT   KELVIN
0 C    = 32 F    = 273 K
10 C    = 50 F    = 283 K
20 C    = 68 F    = 293 K
30 C    = 86 F    = 303 K
40 C    = 104 F    = 313 K
BOILING: 100C = 212F = 373K
TEST 12 COMPLETE
OK
```
- **Execution Time:** 421 ms  
- **Result:** **PASS**

---

### Test 13: Euclidean GCD Algorithm
- **Lines:** 15  
- **Purpose:** Computes the Greatest Common Divisor (GCD) of 252 and 105 using iterative Euclidean modulus reduction.

#### Source Code:
```basic
10 REM P13: EUCLIDEAN GCD
20 A = 252
30 B = 105
40 PRINT "COMPUTING GCD OF ", A, " AND ", B
50 IF B == 0
60   GOTO 140
70 END
80 R = A % B
90 A = B
100 B = R
110 IF B > 0
120  GOTO 80
130 END
140 PRINT "GREATEST COMMON DIVISOR = ", A
150 PRINT "TEST 13 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
COMPUTING GCD OF 252 AND 105
GREATEST COMMON DIVISOR = 21
TEST 13 COMPLETE
OK
```
- **Execution Time:** 266 ms  
- **Result:** **PASS**

---

### Test 14: Collatz 3N+1 Sequence
- **Lines:** 19  
- **Purpose:** Executes the Collatz $3N+1$ hailstone trajectory starting at $N=12$, tracing step by step until converging to 1.

#### Source Code:
```basic
10 REM P14: COLLATZ CONJECTURE
20 N = 12
30 S = 0
40 PRINT "STARTING COLLATZ FOR N = ", N
50 IF N == 1
60   GOTO 140
70 END
80 IF N % 2 == 0
90   N = N / 2
100 ELSE
110  N = 3 * N + 1
120 END
130 S = S + 1
140 PRINT "STEP ", S, " : ", N
150 IF N > 1
160  GOTO 80
170 END
180 PRINT "REACHED 1 IN STEPS: ", S
190 PRINT "TEST 14 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
STARTING COLLATZ FOR N = 12
STEP 1 : 6
STEP 2 : 3
STEP 3 : 10
STEP 4 : 5
STEP 5 : 16
STEP 6 : 8
STEP 7 : 4
STEP 8 : 2
STEP 9 : 1
REACHED 1 IN STEPS: 9
TEST 14 COMPLETE
OK
```
- **Execution Time:** 374 ms  
- **Result:** **PASS**

---

### Test 15: Compound Growth Model
- **Lines:** 12  
- **Purpose:** Models 5 years of compound financial interest on $1,000 principal at 8% annual yield.

#### Source Code:
```basic
10 REM P15: COMPOUND GROWTH
20 P = 1000
30 R = 8
40 PRINT "INITIAL PRINCIPAL: ", P
50 PRINT "ANNUAL RATE: ", R, "%"
60 FOR Y = 1 TO 5
70   I = (P * R) / 100
80   P = P + I
90   PRINT "YEAR ", Y, " INTEREST: ", I, " BALANCE: ", P
100 NEXT Y
110 PRINT "FINAL 5-YEAR BALANCE: ", P
120 PRINT "TEST 15 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
INITIAL PRINCIPAL: 1000
ANNUAL RATE: 8%
YEAR 1 INTEREST: 80 BALANCE: 1080
YEAR 2 INTEREST: 86 BALANCE: 1166
YEAR 3 INTEREST: 93 BALANCE: 1259
YEAR 4 INTEREST: 100 BALANCE: 1359
YEAR 5 INTEREST: 108 BALANCE: 1467
FINAL 5-YEAR BALANCE: 1467
TEST 15 COMPLETE
OK
```
- **Execution Time:** 466 ms  
- **Result:** **PASS**

---

### Test 16: Random Sampling & Statistics
- **Lines:** 18  
- **Purpose:** Generates pseudorandom integers via `RND(100) + 1` and calculates sample sum, average, minimum, and maximum.

#### Source Code:
```basic
10 REM P16: STATISTICAL SAMPLING
20 S = 0
30 H = 0
40 L = 1000
50 FOR I = 1 TO 8
60   V = RND(100) + 1
70   S = S + V
80   IF V > H
90     H = V
100  END
110  IF V < L
120    L = V
130  END
140 NEXT I
150 A = S / 8
160 PRINT "SAMPLES: 8, SUM: ", S, " AVG: ", A
170 PRINT "MIN: ", L, " MAX: ", H
180 PRINT "TEST 16 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
SAMPLES: 8, SUM: 506 AVG: 63
MIN: 9 MAX: 94
TEST 16 COMPLETE
OK
```
- **Execution Time:** 249 ms  
- **Result:** **PASS**

---

### Test 17: System Diagnostics & Benchmark
- **Lines:** 16  
- **Purpose:** Measures loop execution time via `MILLIS()`, reads battery voltage in mV via internal ADC (`BATTERY()`), and evaluates `ABS()`.

#### Source Code:
```basic
10 REM P17: SYSTEM DIAGNOSTICS
20 T = MILLIS()
30 S = 0
40 FOR I = 1 TO 200
50   S = S + I
60 NEXT I
70 U = MILLIS()
80 E = U - T
90 B = BATTERY()
100 PRINT "SUM 1..200 = ", S
110 PRINT "TIME ELAPSED (MS): ", E
120 PRINT "BATTERY RAW VALUE: ", B
130 X = 0 - 500
140 M = ABS(X)
150 PRINT "ABS(-500) = ", M
160 PRINT "TEST 17 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
SUM 1..200 = 20100
TIME ELAPSED (MS): 69
BATTERY RAW VALUE: 3443
ABS(-500) = 500
TEST 17 COMPLETE
OK
```
- **Execution Time:** 358 ms  
- **Result:** **PASS**

---

### Test 18: Countdown Engine with DELAY
- **Lines:** 10  
- **Purpose:** Validates hardware sleep timers and non-blocking execution using `DELAY` and descending `STEP -1`.

#### Source Code:
```basic
10 REM P18: COUNTDOWN TIMER
20 PRINT "ROCKET LAUNCH COUNTDOWN:"
30 FOR C = 5 TO 1 STEP -1
40   PRINT "T MINUS: ", C
50   DELAY 100
60 NEXT C
70 PRINT "IGNITION... ENGINES RUNNING"
80 DELAY 100
90 PRINT "LIFTOFF! WE HAVE A LIFTOFF!"
100 PRINT "TEST 18 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
ROCKET LAUNCH COUNTDOWN:
T MINUS: 5
T MINUS: 4
T MINUS: 3
T MINUS: 2
T MINUS: 1
IGNITION... ENGINES RUNNING
LIFTOFF! WE HAVE A LIFTOFF!
TEST 18 COMPLETE
OK
```
- **Execution Time:** 829 ms  
- **Result:** **PASS**

---

### Test 19: Pythagorean Triples Discovery
- **Lines:** 15  
- **Purpose:** Executes a three-level nested search loop to find all integer solutions to $A^2 + B^2 = H^2$ for $A \le 15, B \le 20, H \le 25$.

#### Source Code:
```basic
10 REM P19: PYTHAGOREAN TRIPLES
20 PRINT "FINDING PYTHAGOREAN TRIPLES <= 20:"
30 C = 0
40 FOR A = 3 TO 15
50   FOR B = A TO 20
60     FOR H = B TO 25
70       IF (A * A) + (B * B) == (H * H)
80         PRINT A, "^2 + ", B, "^2 = ", H, "^2"
90         C = C + 1
100      END
110    NEXT H
120  NEXT B
130 NEXT A
140 PRINT "FOUND TOTAL TRIPLES: ", C
150 PRINT "TEST 19 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
FINDING PYTHAGOREAN TRIPLES <= 20:
3^2 + 4^2 = 5^2
5^2 + 12^2 = 13^2
6^2 + 8^2 = 10^2
8^2 + 15^2 = 17^2
9^2 + 12^2 = 15^2
12^2 + 16^2 = 20^2
15^2 + 20^2 = 25^2
FOUND TOTAL TRIPLES: 7
TEST 19 COMPLETE
OK
```
- **Execution Time:** 2004 ms  
- **Result:** **PASS**

---

### Test 20: Digital Logic Truth Tables
- **Lines:** 14  
- **Purpose:** Generates 2-input truth tables for fundamental digital logic gates: AND (`&`), OR (`|`), and XOR (`^`).

#### Source Code:
```basic
10 REM P20: DIGITAL LOGIC GATES
20 PRINT "A  B  AND  OR  XOR"
30 PRINT "------------------"
40 FOR A = 0 TO 1
50   FOR B = 0 TO 1
60     P = A & B
70     Q = A | B
80     R = A ^ B
90     PRINT A, "  ", B, "   ", P, "   ", Q, "   ", R
100  NEXT B
110 NEXT A
120 PRINT "------------------"
130 PRINT "ALL GATES VERIFIED"
140 PRINT "TEST 20 COMPLETE"
```

#### Terminal Output (`RUN`):
```text
A  B  AND  OR  XOR
------------------
0  0   0   0   0
0  1   0   1   1
1  0   0   1   1
1  1   1   1   0
------------------
ALL GATES VERIFIED
TEST 20 COMPLETE
OK
```
- **Execution Time:** 377 ms  
- **Result:** **PASS**

---

## 4. Key Findings & Technical Rules for BlueBASIC

During the creation and verification of these 20 programs, several architectural and syntax characteristics of the BlueBASIC interpreter were identified and documented:

1. **Scalar Variable Naming:**
   BlueBASIC supports only single-character variable names (`A` through `Z`). Compound names like `R1` or `VAR` are parsed as separate tokens, which can produce syntax errors (`Bad expression`).

2. **Loop Initialization & Evaluation:**
   In BlueBASIC, a `FOR` loop evaluates its termination condition upon reaching `NEXT`. The loop body is entered immediately upon the `FOR` statement. For loops that might need zero iterations, an explicit `IF` guard must precede the `FOR` loop.

3. **Built-in Functions:**
   - `MILLIS()` returns elapsed milliseconds since boot as a 32-bit unsigned long.
   - `BATTERY()` samples the internal bandgap reference against $V_{DD}/3$ via `ADCCON3` and returns power rail voltage directly in millivolts ($3443\text{ mV} \approx 3.44\text{ V}$).
   - `ABS(X)` computes absolute value. Unary minus should be assigned to an intermediate variable or written as `0 - X`.
   - `RND(N)` returns a pseudo-random integer in the range $[0, N-1]$.

4. **Flash Memory Architecture:**
   - Program lines are committed to flash pages starting at CPU address `0x9000` (DMA physical address `0x29000`).
   - `NEW` erases the flash pages and sets line pointers back to start.
   - Re-entering an existing line number automatically invalidates and overwrites that line in flash memory.

---

## 5. Conclusion

The serial communication link between the host PC and the CC2541 HM-10 module over UART1 Alternative 2 (`P1_6`/`P1_7`) is completely stable, robust, and validated. 

By eliminating the clock-divide-on-halt behavior (`HCI_EXT_DISABLE_CLK_DIVIDE_ON_HALT`) and implementing echo-synchronized flow control, BlueBASIC achieved a **flawless 20 out of 20 test pass rate** with 100% memory persistence and exact numerical accuracy.
