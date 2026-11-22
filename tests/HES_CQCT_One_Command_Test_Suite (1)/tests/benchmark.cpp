#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include "../src/crypto.cpp"

using namespace std;
using Clock = chrono::steady_clock;

int main() {
    const string K1 = "AlphaKey123";
    const string K2 = "BetaShift456";
    const string K3 = "GammaMix789";

    // false disables the original debug output so console I/O does not distort timings.
    CryptoEngine crypto(K1, K2, K3, false);

    const size_t dataSize = 1024 * 1024; // 1 MiB
    const int iterations = 10;
    const long long nonce = 123456;
    const string message(dataSize, 'A');

    // Warm-up calls (not included in the measurements).
    string warmCipher = crypto.encryptMsg(message.substr(0, 1024), nonce);
    crypto.decryptMsg(warmCipher, nonce);

    double encryptSeconds = 0.0;
    double decryptSeconds = 0.0;
    string cipher, recovered;

    for (int i = 0; i < iterations; ++i) {
        auto start = Clock::now();
        cipher = crypto.encryptMsg(message, nonce);
        auto stop = Clock::now();
        encryptSeconds += chrono::duration<double>(stop - start).count();

        start = Clock::now();
        recovered = crypto.decryptMsg(cipher, nonce);
        stop = Clock::now();
        decryptSeconds += chrono::duration<double>(stop - start).count();

        if (recovered != message) {
            cerr << "ERROR: decrypted data did not match original input.\n";
            return 1;
        }
    }

    const double totalMiB = (static_cast<double>(dataSize) * iterations) / (1024.0 * 1024.0);
    const double avgEnc = encryptSeconds / iterations;
    const double avgDec = decryptSeconds / iterations;
    const double encMiBs = totalMiB / encryptSeconds;
    const double decMiBs = totalMiB / decryptSeconds;

    ofstream csv("benchmark_results.csv");
    csv << "operation,data_bytes,iterations,total_seconds,average_seconds,throughput_MiB_per_second\n";
    csv << fixed << setprecision(6);
    csv << "encryption," << dataSize << "," << iterations << "," << encryptSeconds << "," << avgEnc << "," << encMiBs << "\n";
    csv << "decryption," << dataSize << "," << iterations << "," << decryptSeconds << "," << avgDec << "," << decMiBs << "\n";
    csv.close();

    cout << fixed << setprecision(6);
    cout << "HES-CQCT C++ benchmark\n";
    cout << "Payload: 1 MiB | Iterations: " << iterations << "\n";
    cout << "Average encryption time: " << avgEnc << " seconds\n";
    cout << "Encryption throughput: " << encMiBs << " MiB/s\n";
    cout << "Average decryption time: " << avgDec << " seconds\n";
    cout << "Decryption throughput: " << decMiBs << " MiB/s\n";
    cout << "Round-trip verification: PASS\n";
    cout << "CSV written to benchmark_results.csv\n";
}
