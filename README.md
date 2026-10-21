# HES-CQCT — Hybrid Encryption Scheme over a Circular Queue Comm. Test

A small C++ project that demonstrates:

- A **custom three-layer cipher** (`CryptoEngine`): XOR mixing (`K1`), a dynamic shift (`K2`), and a nonce-dependent string rotation (`K3`), for encrypting/decrypting short messages with a per-message nonce.
- A **circular singly-linked queue** (`CircularQueue`) used as the "transport" that carries `(ciphertext, nonce)` pairs from sender to receiver.

> **Status:** Phase 3 — 100% complete. All three keys (`K1`, `K2`, `K3`) are now active, and every encryption layer is printed as a hex dump for inspection.

## Project structure

```
HES-CQCT/
├── include/
│   ├── CryptoEngine.h      # cipher class declaration
│   └── CircularQueue.h     # queue struct + function declarations
├── src/
│   ├── CryptoEngine.cpp    # cipher implementation (K1/K2/K3 + nonce generation)
│   ├── CircularQueue.cpp   # queue implementation
│   └── main.cpp            # menu-driven demo driver: encrypt -> enqueue -> decrypt
├── Makefile
├── .gitignore
└── README.md
```

## Build & run

```bash
make
./hes_cqct
```

You'll get an interactive menu:
```
1. Encrypt & Enqueue Message
2. Decrypt All Messages in Queue
3. Exit
```
Pick `1` to type a message (it gets encrypted and pushed into the queue), repeat as many times as you like, then pick `2` to decrypt every message currently in the queue in order.

Or compile manually:

```bash
g++ -std=c++17 -Wall -Iinclude -o hes_cqct src/main.cpp src/CryptoEngine.cpp src/CircularQueue.cpp
```

## How it works

1. `CryptoEngine::generateNonce()` produces a 6-digit nonce using `std::mt19937_64` seeded from the current system clock reading (`std::chrono::system_clock`).
2. `encryptMsg()` applies three layers, each printed as a hex dump so you can see the effect of each step in turn:
   - **K1 (XOR mixing):** each character is XORed with a key byte, the nonce, and its own index.
   - **K2 (dynamic shift):** each character is shifted forward by an amount derived from a key byte plus the nonce.
   - **K3 (rotation):** the whole string is rotated left or right by `r` positions, where `r` is derived from the sum of `K3`'s ASCII values plus the nonce, modulo the message length. Even `r` rotates right, odd `r` rotates left.
3. The final ciphertext and its nonce are pushed into a **circular queue** via `enqueue()`.
4. The menu lets you enqueue any number of encrypted messages, then process the whole queue at once: starting at `front`, `decryptMsg()` reverses the three layers in opposite order (undo K3's rotation, then reverse K2's shift, then reverse K1's XOR) for each node in turn until it loops back to `front`.

## Known limitations (good next steps)

- **Nonce source is weaker than it could be.** `generateNonce()` seeds from the system clock (`std::chrono::system_clock`) rather than an OS entropy source like `std::random_device`. A clock reading is far easier to guess or reproduce — if nonce unpredictability matters for the project's security claims, this is worth a deliberate decision either way, flagged explicitly in the report.
- **This cipher is for learning, not for securing real data.** Even with three layers active, it hasn't been analyzed for weaknesses (e.g. key reuse across positions via modulo indexing, predictable rotation derived from a public-ish key sum, small nonce space). This is best framed as a DSA + cipher-logic exercise, not a production crypto primitive.
- **No true `dequeue()` yet** — "Decrypt All Messages" reads every node by walking the circular links but doesn't remove any of them from the queue, so the queue keeps growing across repeated runs of option 1.
- **Keys are hardcoded** in `main.cpp`. For anything beyond a demo, keys shouldn't live in source code.
- **Edge case worth knowing:** when `r` (the K3 rotation amount) comes out to 0, no rotation happens — this is handled correctly, but it's worth testing a few messages in your demo that land on `r = 0` vs. `r > 0` so graders see both code paths run.

## Contributors

| Member | Role | Technical Contribution | GitHub Commits |
|---|---|---|---|
| Srishti Tilara | Team Lead | Integration, K1/K2/K3 logic, testing | 2 |
| Tejas Pratap Singh | Performance and Documentation | Benchmarking, report, GitHub | 3 |
| Hrithik Mehta | Cryptography | Encryption, decryption, CSPRNG | 3 |
| Priyanka Rawat | Rotation Module | Dynamic shift, queue rotation | 3 |

## License

Add a license of your choice (e.g. MIT) before publishing if you want others to be able to reuse this.
