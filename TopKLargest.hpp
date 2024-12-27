#include "Heap.hpp"
#include <vector>

template <typename T, typename C>
struct TopKLargest {
    Heap<T, C> heap;
    size_t k = 0;

    TopKLargest(size_t k) : k(k) {}

    void add(const T& value) {
        if (heap.count < k) {
            T copied = value;
            heap.add(std::move(copied));
            return;
        }

        if (heap.comparer.compare(heap.getTop(), value) < 0) {
            heap.removeTop();
            T copied = value;
            heap.add(std::move(copied));
        }
    }

    size_t getCount() {
        return heap.count;
    }

    bool isEmpty() {
        return heap.isEmpty();
    }

    T removeTop() {
        return heap.removeTop();
    }
};