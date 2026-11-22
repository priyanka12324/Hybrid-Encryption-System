#include <iostream>
#include <string>
#include <cstdint>
#include "../src/crypto.cpp"

static int bitCount(unsigned char x) {
    int count = 0;
    while (x) { count += (x & 1); x >>= 1; }
    return count;
}

int main() {
    CryptoEngine crypto("AlphaKey123", "BetaShift456", "GammaMix789", false);
    const long long nonce = 456789;
    const std::string original = "HES-CQCT diffusion test message";
    std::string changed = original;
    changed[0] = static_cast<char>(static_cast<unsigned char>(changed[0]) ^ 0x01);

    const std::string c1 = crypto.encryptMsg(original, nonce);
    const std::string c2 = crypto.encryptMsg(changed, nonce);
    size_t changedBytes = 0;
    int changedBits = 0;
    const size_t n = std::min(c1.size(), c2.size());

    for (size_t i = 0; i < n; ++i) {
        unsigned char a = static_cast<unsigned char>(c1[i]);
        unsigned char b = static_cast<unsigned char>(c2[i]);
        if (a != b) ++changedBytes;
        changedBits += bitCount(static_cast<unsigned char>(a ^ b));
    }

    std::cout << "Plaintext inputs differ by one bit at the first byte.\n";
    std::cout << "Ciphertext bytes changed: " << changedBytes << " / " << c1.size() << "\n";
    std::cout << "Ciphertext bits changed: " << changedBits << " / " << (c1.size() * 8) << "\n";
    std::cout << "This is an observed diffusion measurement, not a pass/fail security test.\n";
}
