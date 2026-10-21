#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include <string>

// A single message slot in the circular queue.
struct Node {
    std::string c_txt;   // ciphertext
    long long nonce;      // nonce used to encrypt this message
    Node* next;
};

// Circular queue: rear->next always points back to front.
struct CircularQueue {
    Node* front;
    Node* rear;
};

CircularQueue* createQueue();
Node* createNode(std::string cipher, long long val);
void enqueue(CircularQueue* q, std::string cipher, long long val);
void displayQueue(CircularQueue* q);

#endif // CIRCULAR_QUEUE_H
