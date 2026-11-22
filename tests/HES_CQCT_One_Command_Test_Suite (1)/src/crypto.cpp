#ifndef HES_CQCT_CRYPTO_CPP
#define HES_CQCT_CRYPTO_CPP

#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <iomanip>

using namespace std;

class CryptoEngine {
private:
    string K1, K2, K3;
    bool verbose;

public:
    CryptoEngine(string key1, string key2, string key3, bool showDebug = true)
        : K1(key1), K2(key2), K3(key3), verbose(showDebug) {}

    long long generateNonce() {
        // Kept compatible with the original project implementation.
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        mt19937_64 gen(seed);
        uniform_int_distribution<long long> dist(100000, 999999);
        return dist(gen);
    }

    string encryptMsg(string text, long long nonce) {
        string result = text;
        string k1_state = text;

        for (size_t i = 0; i < text.length(); i++) {
            char k1Char = K1[i % K1.length()];
            result[i] = result[i] ^ k1Char ^ (nonce % 256) ^ (i % 256);
            k1_state[i] = result[i];

            char k2Char = K2[i % K2.length()];
            int shift = (k2Char + nonce) % 256;
            result[i] = result[i] + shift;
        }

        if (verbose) {
            cout << "K1 Layer Encrypted Text: ";
            for (unsigned char c : k1_state)
                cout << hex << uppercase << setw(2) << setfill('0') << (int)c << " ";
            cout << dec << "(" << k1_state << ")\n";

            cout << "K2 Layer Encrypted Text: ";
            for (unsigned char c : result)
                cout << hex << uppercase << setw(2) << setfill('0') << (int)c << " ";
            cout << dec << "(" << result << ")\n";
        }

        if (text.length() > 0) {
            int k3_sum = 0;
            for (char c : K3) k3_sum += (int)c;

            int r = (k3_sum + (nonce % 256)) % text.length();
            if (r > 0) {
                if (r % 2 == 0) {
                    result = result.substr(result.length() - r) + result.substr(0, result.length() - r);
                    if (verbose) cout << "[K3 Logic] Even: right rotation by " << r << "\n";
                } else {
                    result = result.substr(r) + result.substr(0, r);
                    if (verbose) cout << "[K3 Logic] Odd: left rotation by " << r << "\n";
                }
            } else if (verbose) {
                cout << "[K3 Logic] No rotation needed.\n";
            }
        }
        return result;
    }

    string decryptMsg(string text, long long nonce) {
        string result = text;

        if (text.length() > 0) {
            int k3_sum = 0;
            for (char c : K3) k3_sum += (int)c;

            int r = (k3_sum + (nonce % 256)) % text.length();
            if (r > 0) {
                if (r % 2 == 0)
                    result = result.substr(r) + result.substr(0, r);
                else
                    result = result.substr(result.length() - r) + result.substr(0, result.length() - r);
            }
        }

        for (size_t i = 0; i < text.length(); i++) {
            char k2Char = K2[i % K2.length()];
            int shift = (k2Char + nonce) % 256;
            result[i] = result[i] - shift;

            char k1Char = K1[i % K1.length()];
            result[i] = result[i] ^ k1Char ^ (nonce % 256) ^ (i % 256);
        }
        return result;
    }
};

#endif
