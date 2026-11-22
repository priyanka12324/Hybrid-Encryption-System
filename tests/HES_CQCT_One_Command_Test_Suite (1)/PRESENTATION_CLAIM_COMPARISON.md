# Presentation Claims vs Implemented Test Evidence

| Slide claim | Test added | What it establishes | Remaining qualification |
|---|---|---|---|
| 600 randomized cases | `differential_test.cpp` | 600 C++ engine vs independent C++ reference comparisons, plus 600 round trips | Not C++ vs browser client |
| Plaintext recovery | `differential_test.cpp` | Original bytes recovered for generated cases | Limited to tested inputs |
| Oldest node evicted at capacity | `queue_test.cpp` | Capacity-5 FIFO behavior and eviction of oldest node | Run the test and retain output |
| No memory leaks | `queue_memory_stress.cpp` + AddressSanitizer | Checks exercised queue lifecycle paths for memory errors/leaks | Cannot prove universal leak-freedom; sanitizer availability matters |
| Audit log stores timestamps/nonces/ciphertext | `logger_test.cpp` | Checks event labels, nonce/ciphertext fields and absence of plaintext label | Does not validate relay security |
| Multi-device delivery | No test included | Not established | Requires relay/client code and two actual clients/devices |
| C++ engine and client byte-for-byte parity | No actual client source supplied | Not established | Current differential test is C++ vs C++ reference |
| Encryption/decryption speeds | `benchmark.cpp` | Machine-specific local timing and throughput | Rerun on evaluation machine; benchmark setup may differ from slide |
| 100,000 nonce collision rate | `nonce_test.cpp` | Empirical duplicate count from actual generator | Not a randomness certification; results must be run and recorded |
| One-bit plaintext diffusion | `diffusion_test.cpp` | Changed ciphertext byte/bit count for one selected pair | Not a full cryptanalysis |

## Safe replacement wording for slide 12
“600 repeatable randomized C++ differential comparisons were run against an independent C++ reference implementation. Decryption round-trip checks were also performed. Separate test programs cover queue eviction, audit-log formatting, nonce collision observations and one-bit input sensitivity.”

Do not say “C++ engine and client” until an actual client implementation has been tested against the same fixed input vectors.

## Safe replacement wording for slide 13
“Encryption and decryption throughput were measured locally using a 1 MiB payload over 10 timed iterations, with debug printing disabled. The resulting CSV records the measured performance on the evaluation machine.”

Use the values generated on the actual evaluation laptop, not results from another environment.
