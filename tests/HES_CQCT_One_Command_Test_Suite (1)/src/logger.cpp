#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>

using namespace std;

// Persistent Disk Audit Logger
void logAudit(const string& action, long long nonce, const string& cipher) 
{
    ofstream logFile("audit_log.txt", ios::app);
    if (!logFile.is_open()) 
    {
        return;
    }

    time_t now = time(nullptr);
    tm* ltm = localtime(&now);
    char timeBuf[32];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", ltm);

    stringstream hexStream;
    for (unsigned char c : cipher) 
    {
        hexStream << hex << uppercase << setw(2) << setfill('0') << (int)c;
    }

    logFile << "[" << timeBuf << "] " << action
            << " | NONCE: " << nonce
            << " | CIPHERTEXT: " << hexStream.str() << "\n";
    logFile.close();
}