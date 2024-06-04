#include "chip8vm.hpp"
#include <bitset>
#include <cstddef>
#include <stdexcept>
#include <ranges>
#include <source_location>
#include <format>

namespace  
{
    void unimplemented(const std::source_location& srcLoc)
    {
        throw std::runtime_error {std::format("Unimplemented at {}:{}:{}", srcLoc.file_name(), srcLoc.function_name(), srcLoc.line())};
    }
}

CHIP8::DecodedOpcode::DecodedOpcode(std::uint16_t opcode)
{
    std::bitset<16> opcodeBits {opcode};
    for (auto&& [nibbleIndex, nibble] : nibbles | std::views::enumerate)
    {
        std::bitset<NIBBLE_SIZE> nibbleValue;
        for (const auto bitIndex : std::views::iota(0, NIBBLE_SIZE))
        {
            nibbleValue[bitIndex] = opcodeBits.test(nibbleIndex * NIBBLE_SIZE + bitIndex);
        }
        nibble = nibbleValue.to_ulong();
    }
}

std::uint16_t CHIP8::DecodedOpcode::ToUInt16(size_t nibbleCount) const
{
    if (nibbleCount > nibbles.size())
    {
        const auto srcLoc = std::source_location::current();
        throw std::invalid_argument(std::format("{}:{}:{}: nibbleCount must be less or equal to {}", srcLoc.file_name(), srcLoc.line(), srcLoc.column(), nibbles.size()));
    }

    std::uint16_t result {0};
    for (auto&& [nibbleIndex, nibble] : nibbles | std::views::take(nibbleCount) | std::views::enumerate)
    {
        const auto offset = nibbleIndex * DecodedOpcode::NIBBLE_SIZE;
        result |= (nibble << offset);
    }
    return result;
}

std::byte CHIP8::DecodedOpcode::GetValue() const
{
    return std::byte {static_cast<std::uint8_t>(ToUInt16(2))};
}

std::uint16_t CHIP8::DecodedOpcode::GetAddress() const
{
    return ToUInt16(3);
}

std::pair<std::uint8_t, std::uint8_t> CHIP8::DecodedOpcode::GetRegIndices() const
{
    return {nibbles.at(2), nibbles.at(3)};
}


CHIP8::VirtualMachine::VirtualMachine()
    :
    m_addressRegister(0x000),
    m_programCounter(INITIAL_ADDRESS)
{
    m_displayMemory.fill(0);
    m_registers.fill(std::byte{0});
    m_memory.fill(std::byte{0});

    m_instructionTable.fill(Instruction {&CHIP8::VirtualMachine::UnimplementedInstruction});

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
    const auto decodedOpcode = DecodedOpcode{opcode};
    //find instruction
    const auto instruction = GetInstruction(decodedOpcode);
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

CHIP8::VirtualMachine::Instruction CHIP8::VirtualMachine::GetInstruction(const DecodedOpcode& decodedOpcode) const
{
    return m_instructionTable.at(decodedOpcode.nibbles.back());
}

#pragma region Instructions

void CHIP8::VirtualMachine::UnimplementedInstruction(const DecodedOpcode& decodedOpcode)
{
    throw std::runtime_error(std::format("Encountered unimplemented opcode: {}", decodedOpcode.ToUInt16(decodedOpcode.nibbles.size())));
}

void CHIP8::VirtualMachine::ZeroPrefixInstuctions(const DecodedOpcode& decodedOpcode)
{
    switch (decodedOpcode.nibbles.front()) 
    {   
        //opcode for clear display
        case 0x0:
            ClearDisplay();
        break;

        //opcode for return
        case 0xE:
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

void CHIP8::VirtualMachine::Call(const DecodedOpcode& decodedOpcode)
{
    unimplemented(std::source_location::current());
}

void CHIP8::VirtualMachine::Jump(const DecodedOpcode& decodedOpcode)
{
    std::uint16_t address = decodedOpcode.GetAddress();
    m_programCounter = address;
}

void CHIP8::VirtualMachine::SkipNextInstruction()
{
    m_programCounter += INSTRUCTION_WIDTH;
}

void CHIP8::VirtualMachine::SkipOnRegValEqual(const DecodedOpcode& decodedOpcode)
{
    const auto [registerIndex, _] = decodedOpcode.GetRegIndices();
    const auto value = decodedOpcode.GetValue();
    if (m_registers.at(registerIndex) == value)
    {
        SkipNextInstruction();
    }
}

void CHIP8::VirtualMachine::SkipOnRegValNotEqual(const DecodedOpcode& decodedOpcode)
{
    const auto [registerIndex, _] = decodedOpcode.GetRegIndices();
    const auto value = decodedOpcode.GetValue();
    if (m_registers.at(registerIndex) != value)
    {
        SkipNextInstruction();
    }
}

void CHIP8::VirtualMachine::SkipOnRegsEqual(const DecodedOpcode& decodedOpcode)
{
    const auto regIndices = decodedOpcode.GetRegIndices();
    if (m_registers.at(regIndices.first) == m_registers.at(regIndices.second))
    {
        SkipNextInstruction();
    }
}

void CHIP8::VirtualMachine::SetReg(const DecodedOpcode& decodedOpcode)
{
    const auto [registerIndex, _] = decodedOpcode.GetRegIndices();
    const auto value = decodedOpcode.GetValue();
    m_registers.at(registerIndex) = value;
}

void CHIP8::VirtualMachine::Add(const DecodedOpcode& decodedOpcode)
{
    const auto regIndices = decodedOpcode.GetRegIndices();
    const std::uint8_t additionResult = std::to_integer<std::uint8_t>(m_registers.at(regIndices.first)) + std::to_integer<std::uint8_t>(m_registers.at(regIndices.second));
    m_registers.at(regIndices.first) = std::byte {additionResult};
}

#pragma endregion Instructions