#include <cassert>
#include <iostream>
#include <string>
#include "../src/queue.cpp"

int main() {
    cq* q = createQueue(5);
    assert(q != nullptr);

    for (int i = 1; i <= 5; ++i)
        enqueue(q, "cipher_" + std::to_string(i), 100000 + i);

    assert(q->size == 5);
    assert(q->front->nonce == 100001);
    assert(q->rear->nonce == 100005);
    assert(q->rear->next == q->front);

    // Sixth insertion must evict the oldest node (nonce 100001).
    enqueue(q, "cipher_6", 100006);
    assert(q->size == 5);
    assert(q->front->nonce == 100002);
    assert(q->rear->nonce == 100006);
    assert(q->rear->next == q->front);

    // Verify FIFO order after eviction.
    Node* cur = q->front;
    for (int expected = 2; expected <= 6; ++expected) {
        assert(cur->nonce == 100000 + expected);
        cur = cur->next;
    }
    assert(cur == q->front);

    // Dequeue one and verify size/front update.
    dequeue(q);
    assert(q->size == 4);
    assert(q->front->nonce == 100003);

    destroyQueue(q);
    std::cout << "Queue capacity, circular links, FIFO order, eviction and cleanup: PASS\n";
    return 0;
}
