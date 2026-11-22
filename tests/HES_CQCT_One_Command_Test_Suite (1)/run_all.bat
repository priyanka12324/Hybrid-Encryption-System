@echo off
setlocal EnableExtensions
cd /d "%~dp0"

if not exist build mkdir build
if not exist results mkdir results

echo ============================================================
echo HES-CQCT - BUILD AND RUN ALL TESTS
echo ============================================================

g++ --version
if errorlevel 1 (
  echo ERROR: g++ was not found. Install GCC and add its bin folder to PATH.
  exit /b 1
)

echo.
echo [1/7] Compiling differential test...
g++ -std=c++17 -O2 tests/differential_test.cpp -o build\differential_test.exe
if errorlevel 1 goto compile_error

echo [2/7] Compiling benchmark...
g++ -std=c++17 -O2 tests/benchmark.cpp -o build\benchmark.exe
if errorlevel 1 goto compile_error

echo [3/7] Compiling queue test...
g++ -std=c++17 -O2 tests/queue_test.cpp -o build\queue_test.exe
if errorlevel 1 goto compile_error

echo [4/7] Compiling logger test...
g++ -std=c++17 -O2 tests/logger_test.cpp -o build\logger_test.exe
if errorlevel 1 goto compile_error

echo [5/7] Compiling nonce test...
g++ -std=c++17 -O2 tests/nonce_test.cpp -o build\nonce_test.exe
if errorlevel 1 goto compile_error

echo [6/7] Compiling diffusion test...
g++ -std=c++17 -O2 tests/diffusion_test.cpp -o build\diffusion_test.exe
if errorlevel 1 goto compile_error

echo [7/7] Compiling queue memory stress test...
g++ -std=c++17 -O2 tests/queue_memory_stress.cpp -o build\queue_memory_stress.exe
if errorlevel 1 goto compile_error

echo.
echo ============================================================
echo RUNNING TESTS
echo ============================================================

 echo.
echo ---------- 1. DIFFERENTIAL TEST (600 CASES) ----------
build\differential_test.exe
if errorlevel 1 goto test_error

echo.
echo ---------- 2. BENCHMARK ----------
build\benchmark.exe
if errorlevel 1 goto test_error
if exist benchmark_results.csv copy /Y benchmark_results.csv results\benchmark_results.csv >nul

echo.
echo ---------- 3. QUEUE TEST ----------
build\queue_test.exe
if errorlevel 1 goto test_error

echo.
echo ---------- 4. LOGGER TEST ----------
build\logger_test.exe
if errorlevel 1 goto test_error
if exist audit_log.txt copy /Y audit_log.txt results\audit_log.txt >nul

echo.
echo ---------- 5. NONCE OBSERVATION (100,000 CALLS) ----------
build\nonce_test.exe
if errorlevel 1 goto test_error

echo.
echo ---------- 6. DIFFUSION OBSERVATION ----------
build\diffusion_test.exe
if errorlevel 1 goto test_error

echo.
echo ---------- 7. QUEUE MEMORY STRESS ----------
build\queue_memory_stress.exe
if errorlevel 1 goto test_error

echo.
echo ============================================================
echo ALL 7 TEST PROGRAMS COMPLETED SUCCESSFULLY
echo Benchmark CSV and audit log (if generated) are in results\
echo NOTE: Memory stress was run without AddressSanitizer.
echo NOTE: Differential test compares C++ with a C++ reference,
echo       not the browser JavaScript client.
echo ============================================================
exit /b 0

:compile_error
echo.
echo BUILD FAILED. Fix the compiler error shown above.
exit /b 1

:test_error
echo.
echo A TEST FAILED. See the output above; later tests were not run.
exit /b 1
