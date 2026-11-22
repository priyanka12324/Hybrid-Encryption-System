#include <iostream>
#include <sstream>
#include <string>
#include "../src/queue.cpp"

int main() {
    // Keep the test readable: queue.cpp prints on every enqueue/eviction.
    std::ostringstream suppressed;
    std::streambuf* original = std::cout.rdbuf(suppressed.rdbuf());

    const int rounds = 50;
    const int messagesPerRound = 50;

    for (int round = 0; round < rounds; ++round) {
        cq* q = createQueue(5);
        for (int i = 0; i < messagesPerRound; ++i) {
            enqueue(q, "memory_test_ciphertext", 100000 + round * messagesPerRound + i);
        }
        for (int i = 0; i < 3; ++i) {
            dequeue(q);
        }
        destroyQueue(q);
    }

    std::cout.rdbuf(original);
    std::cout << "Queue memory stress workload: PASS\n";
    std::cout << "Rounds: " << rounds << " | messages per round: " << messagesPerRound << "\n";
    std::cout << "Allocation, eviction, dequeue and destruction exercised.\n";
    std::cout << "Note: use AddressSanitizer separately for memory-error detection.\n";
    return 0;
}
