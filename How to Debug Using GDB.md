# 🐞 Debugging a Program with a Logical Error using GDB

This guide walks through debugging a C++ program that contains **logical errors**.  
The program is supposed to compute:

\[
(X^0)/0! + (X^1)/1! + (X^2)/2! + (X^3)/3! + (X^4)/4! + \dots + (X^n)/n!
\]

for given inputs `x` and `n`.  
However, it **always outputs infinity** (`inf`), regardless of the input.

We will trace the issue step by step using **GDB**.

---

## 📂 Step 1: Get the Sample Program
Download the buggy program:

➡️ [`broken.cpp`](https://cs.baylor.edu/~donahoo/tools/gdb/broken.cpp)

## ⚙️ Step 2: Compile and Run

Compile with debugging info:
g++ -g broken.cpp -o broken
./broken

## 🐞 Step 3: Start GDB
$gdb broken

## 🎯 Step 4: Set a Breakpoint

Set a breakpoint at line 43:

(gdb) b 43

This corresponds to:
double seriesValue = ComputeSeriesValue(x, n);

## ▶️ Step 5: Run Program in Debugger

Run the program:

(gdb) run

When prompted, enter:

x = 2
n = 3

>> Expected output: 5

Debugger stops at the breakpoint:
Breakpoint 1, main () at broken.cpp:43
43  double seriesValue = ComputeSeriesValue(x, n);

## 🧭 Step 6: Step Into ComputeSeriesValue

To step into the function:

(gdb) step

Execution moves to:
17  double seriesValue=0.0;

Continue stepping:
(gdb) next
18  double xpow=1;
(gdb) n
20  for (int k = 0; k <= n; k++) {
(gdb) n
21    seriesValue += xpow / ComputeFactorial(k) ;
(gdb) step

Now inside ComputeFactorial():
7  int fact=0;   // ⚠️ suspicious

## 🔍 Step 7: Inspect Execution

Use backtrace to see where we are:

(gdb) bt
#0  ComputeFactorial (number=0) at broken.cpp:7
#1  0x08048907 in ComputeSeriesValue (x=2, n=3) at broken.cpp:21
#2  0x08048a31 in main () at broken.cpp:43


Step through the loop:

(gdb) next
9  for (int j = 0; j <= number; j++) {
(gdb) n
10    fact = fact * j;
(gdb) n
9  for (int j = 0; j <= number; j++) {


Check variable value:

(gdb) print fact
$2 = 0
Continue:
(gdb) n
13  return fact;
(gdb) quit

>> 🚨 Bug Found

fact was initialized as 0
Factorial is computed as:
fact = fact * j;

---



