#include <string>
#include <algorithm>
#include <random>
#include <set>
#include "RBTree.hpp"

struct Buku {
    std::string nama;

};

std::string BukuNamaKey(const Buku& buku) {
    return buku.nama;
}

int main()
{
    RBTree<Buku, decltype(BukuNamaKey)*> rb(&BukuNamaKey);

    std::vector<std::string> strs;

    for (char ch = 'A'; ch <= 'Z'; ch++)
    {
        strs.push_back(std::string{ch});
    }

    std::random_device rd;
    std::mt19937 rng(rd());

    // std::shuffle(strs.begin(), strs.end(), rng);

    for (auto ch : strs)
    {
        std::cout << "Test for " << std::string{ch} << std::endl;
        rb.insert(Buku{ch});

        std::cout << "Max Level " << rb.maxLevel(rb.root, 0) << std::endl;
        Buku last{""};
        auto visitor = [&](RBNode<Buku> *node)
        {
            if (BukuNamaKey(node->value) < BukuNamaKey(last))
            {
                throw std::domain_error("Aneh");
            }
            last = node->value;
        };
        rb.inorder(rb.root, visitor);
    }

    rb.findBetween("F", "X", [](RBNode<Buku> *node)
                   { std::cout << node->value.nama << std::endl; });
}