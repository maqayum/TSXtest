#include <cstdlib>
#include <iostream>
#include <thread>

void task1(const std::string msg)
{
    std::cout << "task1 says: " << msg << std::endl;
}

int main(int argc, char **argv)
{
    std::thread t1(task1, "Hello");
    t1.join();

    return EXIT_SUCCESS;
}
