#pragma once

#include <array>
#include <bitset>
#include <bit>
#include <functional>
#include <cstddef>
#include <cstdint>

namespace CHIP8
{
    class VirtualMachine
    {
        static constexpr unsigned int 
            MEMORY_SIZE = 4096, 
            REGISTER_COUNT = 16,
            DISPLAY_WIDTH = 64,
            DISPLAY_HEIGHT = 32,
            ADDRESS_BUS_WIDTH = std::bit_width(MEMORY_SIZE),
            INITIAL_ADDRESS = 0x200,
            INSTRUCTION_WIDTH = 2;
        
        using Instruction = std::function<void(VirtualMachine* vm, std::uint16_t)>;

        std::array<std::byte, MEMORY_SIZE> m_memory;
        std::array<std::byte, REGISTER_COUNT> m_registers;
        std::uint16_t m_addressRegister, m_programCounter;
        std::array<std::bitset<DISPLAY_WIDTH>, DISPLAY_HEIGHT> m_displayMemory;
        std::array<Instruction, 16> m_instructionTable;

        std::uint16_t FetchNextInstruction() const;
        Instruction Decode(std::uint16_t opcode) const;

        /*Instructions*/ 

        void ZeroPrefixInstuctions(std::uint16_t opcode);
        void ClearDisplay();

        void Jump(std::uint16_t opcode);
    
        void Call(std::uint16_t opcode);

        void SkipNextInstruction();
        void SkipOnRegValEqual(std::uint16_t opcode);

    public:
        VirtualMachine();
        [[deprecated("Unimplemented")]]
        void OnClock();
    };
}