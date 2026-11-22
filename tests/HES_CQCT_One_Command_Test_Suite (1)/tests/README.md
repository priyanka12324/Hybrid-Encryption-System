# HES-CQCT C++ Testing Suite

## Scope
This suite tests the uploaded C++ implementation. It does not fabricate browser-client or multi-device test results. Every test must be run locally; machine-specific results are not hard-coded.

## Layout
- `src/crypto.cpp`: testing-compatible copy of the project's CryptoEngine. It preserves the existing algorithm and adds optional `showDebug=false` for timing. The default remains verbose.
- `src/queue.cpp`, `src/logger.cpp`: project queue and logger source.
- `tests/differential_test.cpp`: 600 randomized C++-to-C++ reference comparisons plus round-trip checks.
- `tests/benchmark.cpp`: 1 MiB, 10-iteration encryption/decryption benchmark; writes CSV.
- `tests/queue_test.cpp`: capacity, circular-link, FIFO and oldest-node eviction checks.
- `tests/queue_memory_stress.cpp`: repeated queue allocation/eviction/destruction workload for sanitizer runs.
- `tests/logger_test.cpp`: verifies expected audit log fields and event labels.
- `tests/nonce_test.cpp`: calls the project's actual nonce generator 100,000 times and reports observed duplicates.
- `tests/diffusion_test.cpp`: flips one plaintext bit and reports ciphertext bit/byte changes.

## Requirements
G++ with C++17 support. Run commands from the repository root (the folder containing `src` and `tests`).

## Build and run (Windows PowerShell)
```powershell
g++ -std=c++17 -O2 tests/differential_test.cpp -o differential_test.exe
.\differential_test.exe

g++ -std=c++17 -O2 tests/benchmark.cpp -o benchmark.exe
.\benchmark.exe

g++ -std=c++17 -O2 tests/queue_test.cpp -o queue_test.exe
.\queue_test.exe

g++ -std=c++17 -O2 tests/logger_test.cpp -o logger_test.exe
.\logger_test.exe

g++ -std=c++17 -O2 tests/nonce_test.cpp -o nonce_test.exe
.\nonce_test.exe

g++ -std=c++17 -O2 tests/diffusion_test.cpp -o diffusion_test.exe
.\diffusion_test.exe

g++ -std=c++17 -O1 -g -fsanitize=address tests/queue_memory_stress.cpp -o queue_memory_stress.exe
.\queue_memory_stress.exe
```

If GCC on Windows does not support AddressSanitizer in your installation, run the sanitizer command in WSL/Linux or use an available memory diagnostic tool. A completed stress run without sanitizer support alone does not prove there are no leaks.

## Test descriptions and interpretation

### Differential testing
For each of 600 cases, the project C++ `CryptoEngine` and an independently written C++ reference function receive the same bytes, keys and nonce. Their ciphertexts are compared byte-for-byte. The project engine then decrypts its ciphertext and checks the recovered bytes against the original input. The fixed PRNG seed `20261122` makes the test inputs repeatable.

This is **not** a C++ versus browser-client test. No browser/JavaScript client source was supplied for this package.

### Benchmarking
The benchmark measures in-memory encryption/decryption of a 1 MiB payload for 10 iterations, after warm-up. Debug output is disabled during timed calls. It records average seconds and throughput in MiB/s. Results vary by machine/compiler and should be generated on the evaluation laptop.

### Queue test
Checks capacity 5, circular rear-to-front link, FIFO order, oldest-node eviction on sixth enqueue, dequeue and cleanup. It does not by itself prove absence of all memory leaks.

### Memory stress
Repeats queue allocation, enqueue/eviction, dequeue and destruction. Use AddressSanitizer to look for memory errors/leaks. A successful test is evidence only for the exercised paths and tool environment.

### Logger test
Checks that the logger writes event labels, nonce fields and ciphertext fields, and does not write the word `PLAINTEXT`. It tests logger formatting, not relay security or all possible logger failures.

### Nonce test
Uses `CryptoEngine::generateNonce()` exactly as implemented and calls it 100,000 times. It reports the number of distinct values and repeated values observed. This is an empirical collision observation, not a statistical certification or proof of secure randomness. The current generator uses a time-derived seed and a six-digit range; treat the result as a limitation, not as a cryptographic nonce guarantee.

### Diffusion test
Flips one bit in one plaintext byte and counts changed ciphertext bytes/bits for the same nonce and keys. This characterizes this one chosen input pair only; it is not a cryptanalysis proof.

## Claims not verified by this suite
- Actual browser/client versus C++ parity.
- HTTP/SSE relay integration and key isolation in a live relay.
- Multi-device delivery under load.
- Full-system performance.
- Comprehensive memory-leak freedom.
- Cryptographic security.

The relay/client source and an actual second device/session are required to verify the first three claims.

## Presentation wording supported by this suite
“C++ differential testing compared the project's CryptoEngine with an independent C++ reference implementation across 600 repeatable randomized inputs. All generated ciphertext comparisons and decrypt/encrypt round-trip checks passed. Separate tests exercise queue FIFO eviction, audit log formatting, observed nonce collisions and one-bit input sensitivity. Encryption/decryption performance is measured locally and exported to CSV.”

Only include the additional queue/logger/nonce/diffusion results in the presentation after running the corresponding tests and recording their actual output.


## One-command runner (Windows)
From the extracted suite folder, run `run_all.bat` (or ` .\run_all.bat` in PowerShell). It compiles all seven test programs, runs them in order, stops on the first compile/test failure, and copies the benchmark CSV and audit log into `results\`. The queue stress test is intentionally quiet and uses a bounded workload. It runs without AddressSanitizer; a normal successful run is not proof of leak-free memory.
