#ifndef CRYPTO_ENGINE_H
#define CRYPTO_ENGINE_H

#include <string>

// NOTE: This is a hand-rolled educational cipher in three layers:
// K1 (XOR mixing), K2 (nonce-dependent shift), K3 (nonce-dependent
// string rotation). It is NOT a vetted cryptographic algorithm and
// should not be used to protect real/sensitive data (see README).
class CryptoEngine {
private:
    std::string K1;
    std::string K2;
    std::string K3;

public:
    CryptoEngine(std::string key1, std::string key2, std::string key3);

    long long generateNonce();
    std::string encryptMsg(std::string text, long long nonce);
    std::string decryptMsg(std::string text, long long nonce);
};

#endif // CRYPTO_ENGINE_H
