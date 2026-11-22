#include <iostream>
#include <string>
#include <iomanip>
#include "queue.cpp"
#include "crypto.cpp"

using namespace std;

int main() 
{
    cout << "HES-CQCT Phase 3 (100% done)\n";

    CryptoEngine crypto("AlphaKey123", "BetaShift456", "GammaMix789");
    struct cq* mq = createQueue(5);

    int choice;
    do 
    {
        cout << "\n========== MENU ==========\n";
        cout << "1. Encrypt & Enqueue Message\n";
        cout << "2. Decrypt All Messages in Queue\n";
        cout << "3. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) 
        {
            case 1: 
            {
                string p_txt;
                cout << "Enter plain text: ";
                cin >> ws; 
                getline(cin, p_txt); 

                long long curr_n = crypto.generateNonce();
                cout << "\nNonce generated: " << curr_n << "\n";

                string c_txt = crypto.encryptMsg(p_txt, curr_n);
                
                // Print K3 Layer Encrypted Text (Final)
                cout << "K3 Layer Encrypted Text (Final): ";
                for (unsigned char c : c_txt) 
                {
                    cout << hex << uppercase << setw(2) << setfill('0') << (int)c << " ";
                }
                cout << dec << "(" << c_txt << ")\n";

                enqueue(mq, c_txt, curr_n);
                displayQueue(mq);
                break;
            }
            case 2: 
            {
                if (mq->front == nullptr) 
                {
                    cout << "Queue is empty. No messages to decrypt.\n";
                } 
                else 
                {
                    cout << "\n";
                    struct Node* temp = mq->front;
                    int msgCount = 1;
                    
                    do {
                        string dec_txt = crypto.decryptMsg(temp->c_txt, temp->nonce);
                        cout << "[Message " << msgCount++ << "] Decrypted text: " << dec_txt << "\n";
                        temp = temp->next;
                    } while (temp != mq->front);
                }
                break;
            }
            case 3:
                destroyQueue(mq);
                mq = nullptr;
                cout << "End of demo.\n";
                break;
                
            default:
                cout << "Invalid choice. Please select 1, 2, or 3.\n";
        }
    } while (choice != 3);

    return 0;
}
