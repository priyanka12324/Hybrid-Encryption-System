#include "CryptoEngine.h"
#include <random>
#include <chrono>
#include <iostream>
#include <iomanip>

CryptoEngine::CryptoEngine(std::string key1, std::string key2, std::string key3) {
    K1 = key1;
    K2 = key2;
    K3 = key3;
}

// Generating a nonce using the system clock as a seed.
// NOTE: this is weaker than an OS-entropy-based random_device — a clock
// reading is far more guessable/repeatable than true entropy, which
// matters if the nonce needs to resist an attacker who can estimate
// roughly when a message was sent. Worth a mention in the report's
// "future work" / limitations section.
long long CryptoEngine::generateNonce() {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<long long> dist(100000, 999999);
    return dist(gen);
}

// Encryption using K1(XOR), K2 (Dynamic Shift), and K3 (Rotation)
std::string CryptoEngine::encryptMsg(std::string text, long long nonce) {
    std::string result = text;
    std::string k1_state = text;

    for (size_t i = 0; i < text.length(); i++) {
        // K1 XOR Mixing
        char k1Char = K1[i % K1.length()];
        result[i] = result[i] ^ k1Char ^ (nonce % 256) ^ (i % 256);

        k1_state[i] = result[i];

        // K2 Dynamic Shifting
        char k2Char = K2[i % K2.length()];
        int shift = (k2Char + nonce) % 256;
        result[i] = result[i] + shift;
    }

    std::cout << "K1 Layer Encrypted Text: ";
    for (unsigned char c : k1_state) {
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c << " ";
    }
    std::cout << std::dec << "(" << k1_state << ")\n";

    std::cout << "K2 Layer Encrypted Text: ";
    for (unsigned char c : result) {
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c << " ";
    }
    std::cout << std::dec << "(" << result << ")\n";

    // K3 String Rotation based on K3 ASCII sum and nonce
    if (text.length() > 0) {
        int k3_sum = 0;
        for (char c : K3) {
            k3_sum += (int)c;
        }

        int r = (k3_sum + (nonce % 256)) % text.length();

        if (r > 0) {
            if (r % 2 == 0) {
                result = result.substr(result.length() - r) + result.substr(0, result.length() - r);
                std::cout << "[K3 Logic] Shift value r = " << r << " (Even). Right rotating string by " << r << " positions.\n";
            } else {
                result = result.substr(r) + result.substr(0, r);
                std::cout << "[K3 Logic] Shift value r = " << r << " (Odd). Left rotating string by " << r << " positions.\n";
            }
        } else {
            std::cout << "[K3 Logic] Shift value r = 0. No rotation needed.\n";
        }
    }

    return result;
}

// Decryption by performing reverse K3, reverse K2, reverse K1
std::string CryptoEngine::decryptMsg(std::string text, long long nonce) {
    std::string result = text;

    // Reverse K3 String Rotation
    if (text.length() > 0) {
        int k3_sum = 0;
        for (char c : K3) {
            k3_sum += (int)c;
        }

        int r = (k3_sum + (nonce % 256)) % text.length();

        if (r > 0) {
            if (r % 2 == 0) {
                result = result.substr(r) + result.substr(0, r);
            } else {
                result = result.substr(result.length() - r) + result.substr(0, result.length() - r);
            }
        }
    }

    for (size_t i = 0; i < text.length(); i++) {
        // Reverse K2
        char k2Char = K2[i % K2.length()];
        int shift = (k2Char + nonce) % 256;
        result[i] = result[i] - shift;

        // Reverse K1
        char k1Char = K1[i % K1.length()];
        result[i] = result[i] ^ k1Char ^ (nonce % 256) ^ (i % 256);
    }

    return result;
}
