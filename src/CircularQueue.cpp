#include "CircularQueue.h"
#include <iostream>
#include <iomanip>

CircularQueue* createQueue() {
    CircularQueue* q = new CircularQueue();
    q->front = nullptr;
    q->rear = nullptr;
    return q;
}

Node* createNode(std::string cipher, long long val) {
    Node* newNode = new Node();
    newNode->c_txt = cipher;
    newNode->nonce = val;
    newNode->next = nullptr;
    return newNode;
}

void enqueue(CircularQueue* q, std::string cipher, long long val) {
    Node* newNode = createNode(cipher, val);

    if (q->front == nullptr) {
        q->front = newNode;
        q->rear = newNode;
        q->rear->next = q->front;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
        q->rear->next = q->front;
    }
    std::cout << "Added to queue\n";
}

void displayQueue(CircularQueue* q) {
    if (q->front == nullptr) {
        std::cout << "Queue is empty\n";
        return;
    }

    Node* temp = q->front;
    std::cout << "Queue contents:\n";
    do {
        std::cout << " -> Nonce: " << temp->nonce << "\n";
        std::cout << "    Cipher (Hex): ";
        for (unsigned char c : temp->c_txt) {
            std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c << " ";
        }
        std::cout << std::dec << "\n";
        temp = temp->next;
    } while (temp != q->front);
}
