#include "TopKLargest.hpp"
#include <iostream>

struct IntComparer {
    int compare(int a, int b) {
        return a - b;
    }
};

int main() {
    TopKLargest<int, IntComparer> h(4);

    h.add(100);
    h.add(90);
    h.add(120);
    h.add(200);
    h.add(80);
    h.add(50);
    h.add(500);
    h.add(464);
    h.add(65);
    h.add(6);

    while (!h.isEmpty()) {
        std::cout << h.removeTop() << std::endl;
    }
}