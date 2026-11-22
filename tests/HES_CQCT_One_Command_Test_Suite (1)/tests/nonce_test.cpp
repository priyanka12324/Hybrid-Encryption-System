#include <iostream>
#include <unordered_set>
#include <vector>
#include "../src/crypto.cpp"

int main() {
    CryptoEngine crypto("AlphaKey123", "BetaShift456", "GammaMix789", false);
    const int count = 100000;
    std::unordered_set<long long> seen;
    int duplicates = 0;
    long long minNonce = 999999999, maxNonce = 0;

    for (int i = 0; i < count; ++i) {
        long long n = crypto.generateNonce();
        if (!seen.insert(n).second) ++duplicates;
        if (n < minNonce) minNonce = n;
        if (n > maxNonce) maxNonce = n;
    }

    double rate = 100.0 * duplicates / count;
    std::cout << "Nonce calls: " << count << "\n";
    std::cout << "Distinct nonce values: " << seen.size() << "\n";
    std::cout << "Duplicate calls: " << duplicates << "\n";
    std::cout << "Duplicate rate: " << rate << "%\n";
    std::cout << "Observed range: " << minNonce << " to " << maxNonce << "\n";
    std::cout << "NOTE: This is an empirical collision check, not a randomness or cryptographic-security test.\n";
}
