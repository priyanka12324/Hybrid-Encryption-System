#include <iostream>
#include <string>
#include <random>
#include <vector>
#include "../src/crypto.cpp"

using namespace std;

// A separately written reference version of the project's three reversible layers.
// It is used as a differential oracle for the existing CryptoEngine.
string referenceEncrypt(const string& input, const string& K1,
                        const string& K2, const string& K3, long long nonce) {
    string out = input;
    for (size_t i = 0; i < out.size(); ++i) {
        unsigned char b = static_cast<unsigned char>(input[i]);
        unsigned char k1 = static_cast<unsigned char>(K1[i % K1.size()]);
        unsigned char k2 = static_cast<unsigned char>(K2[i % K2.size()]);
        b = static_cast<unsigned char>(b ^ k1 ^ (nonce % 256) ^ (i % 256));
        int shift = (static_cast<int>(static_cast<signed char>(k2)) + nonce) % 256;
        b = static_cast<unsigned char>(static_cast<int>(b) + shift);
        out[i] = static_cast<char>(b);
    }

    if (!out.empty()) {
        int sum = 0;
        for (char c : K3) sum += static_cast<int>(c);
        int r = (sum + (nonce % 256)) % static_cast<int>(out.size());
        if (r > 0) {
            if (r % 2 == 0)
                out = out.substr(out.size() - r) + out.substr(0, out.size() - r);
            else
                out = out.substr(r) + out.substr(0, r);
        }
    }
    return out;
}

int main() {
    const string K1 = "AlphaKey123";
    const string K2 = "BetaShift456";
    const string K3 = "GammaMix789";

    CryptoEngine original(K1, K2, K3, false);
    mt19937_64 rng(20261122); // fixed seed makes the test repeatable
    uniform_int_distribution<int> lengthDist(0, 512);
    uniform_int_distribution<int> byteDist(0, 255);
    uniform_int_distribution<long long> nonceDist(100000, 999999);

    const int testCount = 600;
    int encryptionMatches = 0;
    int roundTripPasses = 0;

    for (int t = 0; t < testCount; ++t) {
        int length = lengthDist(rng);
        string message;
        message.reserve(length);
        for (int i = 0; i < length; ++i)
            message.push_back(static_cast<char>(byteDist(rng)));

        long long nonce = nonceDist(rng);
        string actual = original.encryptMsg(message, nonce);
        string expected = referenceEncrypt(message, K1, K2, K3, nonce);

        if (actual == expected) ++encryptionMatches;

        string recovered = original.decryptMsg(actual, nonce);
        if (recovered == message) ++roundTripPasses;

        if (actual != expected || recovered != message) {
            cerr << "FAIL at test " << (t + 1)
                 << " | length=" << length << " | nonce=" << nonce << "\n";
            return 1;
        }
    }

    cout << "Differential tests (project C++ vs independent C++ reference): "
         << encryptionMatches << "/" << testCount << " matched\n";
    cout << "Encrypt/decrypt round trips (project C++ engine): "
         << roundTripPasses << "/" << testCount << " passed\n";
    cout << "RESULT: PASS\n";
    return 0;
}
