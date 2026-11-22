#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include "../src/logger.cpp"

int main() {
    std::remove("audit_log.txt");
    logAudit("ENQUEUE", 456789, std::string("cipher\x01\xAB", 8));
    logAudit("EVICTED", 456790, "ABC");

    std::ifstream in("audit_log.txt");
    assert(in.is_open());
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string contents = buffer.str();

    assert(contents.find("ENQUEUE") != std::string::npos);
    assert(contents.find("EVICTED") != std::string::npos);
    assert(contents.find("NONCE: 456789") != std::string::npos);
    assert(contents.find("NONCE: 456790") != std::string::npos);
    assert(contents.find("CIPHERTEXT:") != std::string::npos);
    assert(contents.find("PLAINTEXT") == std::string::npos);

    std::cout << "Audit log event, nonce and ciphertext checks: PASS\n";
    std::cout << "Note: this test checks the logger output format; it does not prove relay security.\n";
    return 0;
}
