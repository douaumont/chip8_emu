#include "chip8vm.hpp"
#include <bitset>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <ranges>
#include <utility>
#include <source_location>
#include <format>

namespace  
{
    template<typename Integer, typename Count>
    requires(std::is_integral_v<Integer> and std::is_unsigned_v<Integer>)
    Integer ExtractFirstNBits(Integer num, Count count)
    {
        std::bitset<sizeof(Integer) * 8> mask;
        for (const auto bitIndex : std::views::iota(0, count))
        {
            mask[bitIndex] = true;
        }
        return num & (mask.to_ullong());
    }

    template<typename Integer, typename Index>
    requires(std::is_integral_v<Integer> and std::is_unsigned_v<Integer>)
    Integer ExtractNthNibble(Integer num, Index index)
    {
        constexpr auto NIBBLE_SIZE = 4;
        std::bitset<sizeof(num) * 8> mask;
        for (const auto bitIndex : std::views::iota(index * NIBBLE_SIZE, index * NIBBLE_SIZE + NIBBLE_SIZE))
        {
            mask[bitIndex] = true;
        }
        return (num & mask.to_ullong()) >> (index * NIBBLE_SIZE);
    }

    void unimplemented(const std::source_location& srcLoc)
    {
        throw std::runtime_error {std::format("Unimplemented at {}:{}:{}", srcLoc.file_name(), srcLoc.function_name(), srcLoc.line())};
    }
}

CHIP8::VirtualMachine::VirtualMachine()
    :
    m_addressRegister(0x000),
    m_programCounter(INITIAL_ADDRESS)
{
    m_displayMemory.fill(0);
    m_registers.fill(std::byte{0});
    m_memory.fill(std::byte{0});

    m_instructionTable = 
    {
        Instruction {&CHIP8::VirtualMachine::ZeroPrefixInstuctions},
        Instruction {&CHIP8::VirtualMachine::Jump},
        Instruction{},
        Instruction(&CHIP8::VirtualMachine::SkipOnRegValEqual),
    };
}

void CHIP8::VirtualMachine::OnClock()
{
    //fetch instruction
    const auto opcode = FetchNextInstruction();
    //increase value of program counter
    m_programCounter += INSTRUCTION_WIDTH;
    //decode it
    const auto instruction = Decode(opcode);
    //execute it
    instruction(this, opcode);
}

std::uint16_t CHIP8::VirtualMachine::FetchNextInstruction() const
{
    //read first and second bytes which program counter points to
    const auto mostSignificatByte = std::to_integer<std::uint16_t>(m_memory.at(m_programCounter));
    const auto leastSignificantByte = std::to_integer<std::uint16_t>(m_memory.at(m_programCounter + 1));
    //compose opcode value from these bytes
    const std::uint16_t opcodeValue = (mostSignificatByte << 8) | leastSignificantByte;
    return opcodeValue;
}

CHIP8::VirtualMachine::Instruction CHIP8::VirtualMachine::Decode(std::uint16_t opcode) const
{
    const auto prefix = ExtractNthNibble(opcode, 3);
    return m_instructionTable.at(prefix);
}

#pragma region Instructions

void CHIP8::VirtualMachine::ZeroPrefixInstuctions(std::uint16_t opcode)
{
    switch (opcode) 
    {   
        //opcode for clear display
        case 0x00E0:
            ClearDisplay();
        break;

        //opcode for return
        case 0x00EE:
            unimplemented(std::source_location::current());
        break;

        //ignore anything else
        default:
            break;
    }
}

void CHIP8::VirtualMachine::ClearDisplay()
{
    m_displayMemory.fill(0);
}

void CHIP8::VirtualMachine::Call(std::uint16_t opcode)
{
    unimplemented(std::source_location::current());
}

void CHIP8::VirtualMachine::Jump(std::uint16_t opcode)
{
    const auto address = ExtractFirstNBits(opcode, 12);
    m_programCounter = address;
}

void CHIP8::VirtualMachine::SkipNextInstruction()
{
    m_programCounter += INSTRUCTION_WIDTH;
}

void CHIP8::VirtualMachine::SkipOnRegValEqual(std::uint16_t opcode)
{
    const auto registerIndex = ExtractNthNibble(opcode, 2);
    const auto value = std::byte{static_cast<unsigned char>(ExtractFirstNBits(opcode, 8))};
    if (m_registers.at(registerIndex) == value)
    {
        SkipNextInstruction();
    }
}

#pragma endregion Instructions