#include <iostream>
#include <string>
#include <random>
#include <chrono>
#include <iomanip>

using namespace std;

class CryptoEngine 
{
private:
    string K1;
    string K2;
    string K3;

public:
    CryptoEngine(string key1, string key2, string key3) 
    {
        K1 = key1;
        K2 = key2;
        K3 = key3;
    }

    // Generating a nonce using system clock 
    long long generateNonce() 
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        mt19937_64 gen(seed);
        uniform_int_distribution<long long> dist(100000, 999999); 
        return dist(gen);
    }

    // Encryption using K1(XOR), K2 (Dynamic Shift), and K3 (Rotation)
    string encryptMsg(string text, long long nonce) 
    {
        string result = text;
        string k1_state = text; 
        
        for (size_t i = 0; i < text.length(); i++) 
        {
            // K1 XOR Mixing
            char k1Char = K1[i % K1.length()];
            result[i] = result[i] ^ k1Char ^ (nonce % 256) ^ (i % 256);
            
            k1_state[i] = result[i]; 

            // K2 Dynamic Shifting
            char k2Char = K2[i % K2.length()];
            int shift = (k2Char + nonce) % 256; 
            result[i] = result[i] + shift; 
        }

        cout << "K1 Layer Encrypted Text: ";
        for (unsigned char c : k1_state) 
        {
            cout << hex << uppercase << setw(2) << setfill('0') << (int)c << " ";
        }
        cout << dec << "(" << k1_state << ")\n";

        cout << "K2 Layer Encrypted Text: ";
        for (unsigned char c : result) 
        {
            cout << hex << uppercase << setw(2) << setfill('0') << (int)c << " ";
        }
        cout << dec << "(" << result << ")\n";

        // K3 String Rotation based on K3 ASCII sum and nonce
        if (text.length() > 0) 
        {
            int k3_sum = 0;
            for (char c : K3) 
            {
                k3_sum += (int)c;
            }
            
            int r = (k3_sum + (nonce % 256)) % text.length();
            
            if (r > 0) 
            {
                if (r % 2 == 0) 
                {
                    result = result.substr(result.length() - r) + result.substr(0, result.length() - r);
                    cout << "[K3 Logic] Shift value r = " << r << " (Even). Right rotating string by " << r << " positions.\n";
                } 
                else 
                {
                    result = result.substr(r) + result.substr(0, r);
                    cout << "[K3 Logic] Shift value r = " << r << " (Odd). Left rotating string by " << r << " positions.\n";
                }
            }
            else 
            {
                cout << "[K3 Logic] Shift value r = 0. No rotation needed.\n";
            }
        }
        
        return result; 
    }

    // Decryption by performing reverse K3, reverse K2, reverse K1 
    string decryptMsg(string text, long long nonce) 
    {
        string result = text;

        // Reverse K3 String Rotation
        if (text.length() > 0) 
        {
            int k3_sum = 0;
            for (char c : K3) 
            {
                k3_sum += (int)c;
            }
            
            int r = (k3_sum + (nonce % 256)) % text.length();
            
            if (r > 0) 
            {
                if (r % 2 == 0) 
                {
                    result = result.substr(r) + result.substr(0, r);
                } 
                else 
                {
                    result = result.substr(result.length() - r) + result.substr(0, result.length() - r);
                }
            }
        }

        for (size_t i = 0; i < text.length(); i++) 
        {
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
};
