
#define USE_SSE2
#include <ThunderByteScan.hpp>
#include <iostream>
#include <fstream>
#include <chrono>

#define MEASURE(name, x) \
auto start_##name = std::chrono::high_resolution_clock::now(); \
x; \
auto end_##name = std::chrono::high_resolution_clock::now(); \
auto duration_##name = std::chrono::duration_cast<std::chrono::microseconds>(end_##name - start_##name).count(); \
std::cout << #name << " took " << duration_##name << " microseconds.\n";

int main() {
    std::ifstream infile("ThunderByteScan.hpp", std::ios::binary);

    // Check if file was opened successfully
    if (!infile)
    {
        std::cout << "Failed to open file!" << std::endl;
        return 1;
    }

    // Read file contents into vector
    std::vector<char> data((std::istreambuf_iterator<char>(infile)),
        std::istreambuf_iterator<char>());


    uintptr_t result;
    
    MEASURE(LocalFindPatternFirst, ThunderByteScan::LocalFindPatternFirst( "23 70 72 61 67 6D 61 20 6F 6E 63 65 0D 0A 0D 0A" , (uintptr_t)data.data(), (uintptr_t)data.data() + (size_t)data.size(), result));
    return 0;
}