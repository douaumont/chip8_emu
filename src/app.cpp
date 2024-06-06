#include <cstddef>
#include <exception>
#include <vector>
#include <print>
#include <ranges>
#include "chip8/chip8vm.hpp"

int main()
{
    std::vector<std::byte> testProgram {std::byte{0x20}, std::byte{0x00}};
    CHIP8::VirtualMachine vm;
    const auto display = vm.GetDisplayMemory();
    for (const auto row : display)
    {
        std::string rowStr;
        rowStr.reserve(row.size());
        for (const auto pixelIndex : std::views::iota(0UZ, row.size()))
        {
            const auto pixel = row[pixelIndex];
            if (pixel)
            {
                rowStr.push_back('*');
            }
            else 
            {
                rowStr.push_back(' ');
            }
        }
        std::println("{}", rowStr);
    }
    return 0;
}