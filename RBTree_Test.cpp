#include "RBTree.hpp"

struct IntCompare
{
    int compare(const int &a, const int &b)
    {
        return a - b;
    }
};

int main()
{

    RBTree<int, IntCompare> tree;
    for (int y = 1; y <= 128; y++)
    {
        tree.insert(std::move(y));
        std::cout << "Add " << y << ", depth " << tree.maxLevel(tree.root, 0) << std::endl;
    }

    std::cout << "Depth " << tree.maxLevel(tree.root, 0) << std::endl;

    for (int x = 1; x <= 128; x++)
    {
        tree.remove(x);
        std::cout << "Remove " << x << ", depth " << tree.maxLevel(tree.root, 0) << std::endl;
    }
}