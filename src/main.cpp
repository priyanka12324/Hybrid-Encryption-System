#include <iostream>
#include <iomanip>
#include "CryptoEngine.h"
#include "CircularQueue.h"

int main() {
    std::cout << "HES-CQCT Phase 3 (100% done)\n";

    CryptoEngine crypto("AlphaKey123", "BetaShift456", "GammaMix789");
    CircularQueue* mq = createQueue();

    int choice;
    do {
        std::cout << "\n========== MENU ==========\n";
        std::cout << "1. Encrypt & Enqueue Message\n";
        std::cout << "2. Decrypt All Messages in Queue\n";
        std::cout << "3. Exit\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1: {
                std::string p_txt;
                std::cout << "Enter plain text: ";
                std::cin >> std::ws;
                std::getline(std::cin, p_txt);

                long long curr_n = crypto.generateNonce();
                std::cout << "\nNonce generated: " << curr_n << "\n";

                std::string c_txt = crypto.encryptMsg(p_txt, curr_n);

                // Print K3 Layer Encrypted Text (Final)
                std::cout << "K3 Layer Encrypted Text (Final): ";
                for (unsigned char c : c_txt) {
                    std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c << " ";
                }
                std::cout << std::dec << "(" << c_txt << ")\n";

                enqueue(mq, c_txt, curr_n);
                displayQueue(mq);
                break;
            }
            case 2: {
                if (mq->front == nullptr) {
                    std::cout << "Queue is empty. No messages to decrypt.\n";
                } else {
                    std::cout << "\n";
                    Node* temp = mq->front;
                    int msgCount = 1;

                    do {
                        std::string dec_txt = crypto.decryptMsg(temp->c_txt, temp->nonce);
                        std::cout << "[Message " << msgCount++ << "] Decrypted text: " << dec_txt << "\n";
                        temp = temp->next;
                    } while (temp != mq->front);
                }
                break;
            }
            case 3:
                std::cout << "End of demo.\n";
                break;

            default:
                std::cout << "Invalid choice. Please select 1, 2, or 3.\n";
        }
    } while (choice != 3);

    return 0;
}
