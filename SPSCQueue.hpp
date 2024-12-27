#pragma once

#include <atomic>
#include <stdexcept>

/**
 * Single Producer Single Consumer Queue
 * Sebuah yang dikhusukan untuk multithreadeing dengan 1 thread sebagai reader dan 1 thread sebagai writer
 * Queue ini menggunakan implementasi ring buffer dengan array statis
 * 
 * Queue ini menggunakan atomic sebagai metode sinkronisasi
 */

template<typename T>
struct SPSCQueue {
    size_t capacity;
    T* ring;

    using CursorType = std::atomic<size_t>;
    static constexpr size_t align_size = 64;

    alignas(align_size) CursorType _front;
    alignas(align_size) CursorType _back;
    char _padding[align_size - sizeof(size_t)];

    explicit SPSCQueue(size_t capacity) {
        this->capacity = capacity;

        ring = new T[capacity];
    }

    ~SPSCQueue() {
        delete[] ring;
    }

    inline size_t _count(size_t front, size_t back) const {
        return back - front;
    }

    size_t count() const {
        size_t front = _front.load(std::memory_order_relaxed);
        size_t back = _back.load(std::memory_order_relaxed);

        return _count(front, back);
    }

    bool empty() const {
        return count() == 0;
    }

    bool full() const {
        return count() == capacity;
    }

    void push(T&& value) {
        size_t front = _front.load(std::memory_order_acquire);
        size_t back = _back.load(std::memory_order_relaxed);

        if (_count(front, back) == capacity) {
            throw std::overflow_error("Queue is overflow");
        }

        ring[back % capacity] = std::move(value); 
        _back.store(back + 1, std::memory_order_release);
    }

    T pop() {
        size_t front = _front.load(std::memory_order_relaxed);
        size_t back = _back.load(std::memory_order_acquire);

        if (_count(front, back) == 0)
            throw std::underflow_error("Queue is underflow");

        T result = std::move(ring[front % capacity]);
        _front.store(front + 1, std::memory_order_release);
        return result;
    }
};