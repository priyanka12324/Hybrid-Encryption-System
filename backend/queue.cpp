#include <iostream>
#include <string>
#include <iomanip>
#include "logger.cpp"

using namespace std;

struct Node 
{
    string c_txt;
    long long nonce;
    struct Node* next;
};

struct cq 
{
    struct Node* front;
    struct Node* rear;
    int size;
    int max_capacity;
};

// Initializing
struct cq* createQueue(int cap = 5) 
{
    struct cq* q = new cq();
    q->front = nullptr;
    q->rear = nullptr;
    q->size = 0;
    q->max_capacity = cap;
    return q;
}

// Creating a new node
struct Node* createNode(string cipher, long long val) 
{
    struct Node* newNode = new Node();
    newNode->c_txt = cipher;
    newNode->nonce = val;
    newNode->next = nullptr;
    return newNode;
}

// Removing oldest node from front
void dequeue(struct cq* q, bool isEviction = false) 
{
    if (q->front == nullptr) 
    {
        return;
    }

    struct Node* temp = q->front;
    long long removedNonce = temp->nonce;
    string removedCipher = temp->c_txt;

    if (q->front == q->rear) 
    {
        q->front = nullptr;
        q->rear = nullptr;
    } 
    else 
    {
        q->front = q->front->next;
        q->rear->next = q->front;
    }

    delete temp;
    q->size--;

    logAudit(isEviction ? "EVICTED" : "DEQUEUE", removedNonce, removedCipher);
}

void enqueue(struct cq* q, string cipher, long long val) 
{
    if (q->size >= q->max_capacity) 
    {
        cout << "Queue capacity (" << q->max_capacity << ") reached. Evicting oldest message...\n";
        dequeue(q, true);
    }

    struct Node* newNode = createNode(cipher, val);

    if (q->front == nullptr) 
    {
        q->front = newNode;
        q->rear = newNode;
        q->rear->next = q->front; 
    }
    else 
    {
        q->rear->next = newNode;
        q->rear = newNode;
        q->rear->next = q->front; 
    }

    q->size++;
    logAudit("ENQUEUE", val, cipher);
    cout << "Added to queue (" << q->size << "/" << q->max_capacity << ")\n"; 
}

// Freeing all remaining nodes and queue memory
void destroyQueue(struct cq* q) 
{
    if (q == nullptr) 
    {
        return;
    }

    while (q->front != nullptr) 
    {
        struct Node* temp = q->front;
        if (q->front == q->rear) 
        {
            q->front = nullptr;
            q->rear = nullptr;
        } 
        else 
        {
            q->front = q->front->next;
            q->rear->next = q->front;
        }
        delete temp;
    }

    delete q;
}

void displayQueue(struct cq* q) 
{
    if (q->front == nullptr) 
    {
        cout << "Queue is empty\n";
        return;
    }

    struct Node* temp = q->front;
    cout << "Queue contents:\n";
    do {
        cout << " -> Nonce: " << temp->nonce << "\n";
        cout << "    Cipher (Hex): ";
        for (unsigned char c : temp->c_txt) 
        {
            cout << hex << uppercase << setw(2) << setfill('0') << (int)c << " ";
        }
        cout << dec << "\n";
        temp = temp->next;
    } while (temp != q->front);
}
