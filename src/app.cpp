#include <cstddef>
#include <exception>
#include <vector>
#include <print>
#include "chip8/chip8vm.hpp"

int main()
{
    std::vector<std::byte> testProgram {std::byte{0x20}, std::byte{0x00}};
    CHIP8::VirtualMachine vm;
    vm.LoadProgram(testProgram);
    try 
    {
        vm.OnClock();
    } catch (std::exception& excep) 
    {
        std::println("{}", excep.what());
    }
    return 0;
}