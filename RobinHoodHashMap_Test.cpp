#include "RobinHoodHashMap.hpp"
#include "HalfSipHash.h"
#include <stdexcept>

struct IntHasher
{
    uint64_t seed = 0xe17a1465;

    uint64_t hash(int a)
    {
        return HalfSipHash_64(&a, sizeof(a), &seed);
    }
};

int main()
{
    RobinHoodHashMap<int, char, IntHasher> hash;

    auto test = [&]()
    {
        bool mustNot = false;
        for (int i = 0; i < 95; i++)
        {
            char *res = hash.get(i);
            if (res != nullptr)
            {
                if (mustNot) {
                    throw std::domain_error("Must not");
                }
                std::cout << i << "=" << *res << " ";
            } else {
                mustNot = true;
            }
        }

        std::cout << std::endl;
    };

    for (int i = 0; i < 95; i++) {
        hash.put(i, char(' ' + i));
        std::cout << "Add " << i << " > ";
        test();
    }

    for (int i = 94; i >= 0; i--) {
        if (!hash.remove(i)) {
            std::cout << "ANeh" << std::endl;
            return 0;
        }
        std::cout << "Remove " << i << " > ";
        test();
    }
}