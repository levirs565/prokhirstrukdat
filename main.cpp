#include <string>
#include <algorithm>
#include <random>
#include <set>
#include "RBTree.hpp"

struct Buku {
    std::string nama;

};

bool BukuNamaCompare(const Buku& a, const Buku& b) {
    return a.nama < b.nama;
}

int main()
{
    RBTree<Buku, decltype(BukuNamaCompare)*> rb(&BukuNamaCompare);

    std::vector<std::string> strs;

    for (char ch = 'A'; ch <= 'Z'; ch++)
    {
        strs.push_back(std::string{ch});
    }

    std::random_device rd;
    std::mt19937 rng(rd());

    std::shuffle(strs.begin(), strs.end(), rng);

    for (auto ch : strs)
    {
        std::cout << "Test for " << std::string{ch} << std::endl;
        rb.insert(Buku{ch});

        std::cout << "Max Level " << rb.maxLevel(rb.root, 0) << std::endl;
        Buku last{""};
        auto visitor = [&](RBNode<Buku> *node)
        {
            if (BukuNamaCompare(node->value, last))
            {
                throw std::domain_error("Aneh");
            }
            last = node->value;
        };
        rb.inorder(rb.root, visitor);
    }

    rb.insert(Buku{"A"});

    rb.findBetween(Buku{"F"}, Buku{"X"}, [](RBNode<Buku> *node)
                   { std::cout << node->value.nama << std::endl; });
}