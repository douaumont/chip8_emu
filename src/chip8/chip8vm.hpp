#pragma once

#include <array>
#include <bitset>
#include <bit>
#include <functional>
#include <span>
#include <cstddef>
#include <cstdint>

namespace CHIP8
{
    struct DecodedOpcode
    {
        static constexpr auto NIBBLE_SIZE = 4;
        std::array<std::uint8_t, 4> nibbles;

        DecodedOpcode(std::uint16_t opcode);

        std::uint16_t ToUInt16(size_t nibbleCount) const;
        std::byte GetValue() const;
        std::uint16_t GetAddress() const;

        //first = x, second = y
        std::pair<std::uint8_t, std::uint8_t> GetRegIndices() const;
    };

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
        
        using Instruction = std::function<void(VirtualMachine* vm, const DecodedOpcode&)>;

        std::array<std::byte, MEMORY_SIZE> m_memory;
        std::array<std::byte, REGISTER_COUNT> m_registers;
        std::uint16_t m_addressRegister, m_programCounter;
        std::array<std::bitset<DISPLAY_WIDTH>, DISPLAY_HEIGHT> m_displayMemory;

        std::array<Instruction, 16> m_instructionTable;

        std::uint16_t FetchNextInstruction() const;
        Instruction GetInstruction(const DecodedOpcode& decodedOpcode) const;

        /*Instructions*/ 

        void UnimplementedInstruction(const DecodedOpcode& decodedOpcode);

        //prefix = 0
        void ZeroPrefixInstuctions(const DecodedOpcode& decodedOpcode);
        void ClearDisplay();

        //prefix = 1
        void Jump(const DecodedOpcode& decodedOpcode);
    
        //prefix = 2
        void Call(const DecodedOpcode& decodedOpcode);

        //helper function
        void SkipNextInstruction();

        //prefix = 3
        void SkipOnRegValEqual(const DecodedOpcode& decodedOpcode);

        //prefix = 4
        void SkipOnRegValNotEqual(const DecodedOpcode& decodedOpcode);

        //prefix = 5
        void SkipOnRegsEqual(const DecodedOpcode& decodedOpcode);

        //prefix = 6
        void SetReg(const DecodedOpcode& decodedOpcode);

        //prefix = 7
        void Add(const DecodedOpcode& decodedOpcode);

        //prefix = 8
        void EightPrefixInstructions(const DecodedOpcode& decodedOpcode);

        //prefix = 9
        void SkipOnRegsNotEqual(const DecodedOpcode& decodedOpcode);

        //prefix = A
        void SetAddressReg(const DecodedOpcode& decodedOpcode);

        //prefix = B
        void JumpWithOffset(const DecodedOpcode& decodedOpcode);

    public:
        VirtualMachine();
        void LoadProgram(std::span<const std::byte> program);
        void OnClock();
    };
}