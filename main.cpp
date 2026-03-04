#include <iostream>
#include "math_utils.h"

int main(int, char **)
{
    std::cout << "Hello, from cpp-dev!\n";
    std::cout << "2 + 3 = " << add(2, 3) << "\n";
    std::cout << "5! = " << factorial(5) << "\n";
}