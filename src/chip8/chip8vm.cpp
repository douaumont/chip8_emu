#include "chip8vm.hpp"
#include "keyboard.hpp"
#include <algorithm>
#include <bitset>
#include <cstddef>
#include <iterator>
#include <limits>
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

    std::array<std::byte, 3> ToBCD(std::uint8_t n)
    {
        std::array<std::byte, 3> digits;
        
        for (auto& digit : digits)
        {
            digit = std::byte {static_cast<std::uint8_t>(n % 10)};
            n /= 10;
        }

        std::ranges::reverse(digits);

        return digits;
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
    m_programCounter(INITIAL_ADDRESS),
    m_ioCtx(),
    m_delayTimer(m_ioCtx),
    m_soundTimer(m_ioCtx),
    m_clock(m_ioCtx)
{
    m_displayMemory.fill(0);
    m_registers.fill(std::byte{0});
    m_memory.fill(std::byte{0});
    std::ranges::copy(FONT | std::views::transform([](const auto n) {return std::byte{n};}), 
        std::begin(m_memory) + FONT_ADDRESS_START);

    m_instructionTable.fill(Instruction {&CHIP8::VirtualMachine::UnimplementedInstruction});

    m_instructionTable = 
    {
        Instruction {&CHIP8::VirtualMachine::ZeroPrefixInstuctions},
        Instruction {&CHIP8::VirtualMachine::Jump},
        Instruction {&CHIP8::VirtualMachine::Call},
        Instruction {&CHIP8::VirtualMachine::SkipOnRegValEqual},
        Instruction {&CHIP8::VirtualMachine::SkipOnRegValNotEqual},
        Instruction {&CHIP8::VirtualMachine::SkipOnRegsEqual},
        Instruction {&CHIP8::VirtualMachine::SetReg},
        Instruction {&CHIP8::VirtualMachine::Add},
        Instruction {&CHIP8::VirtualMachine::EightPrefixInstructions},
        Instruction {&CHIP8::VirtualMachine::SkipOnRegsNotEqual},
        Instruction {&CHIP8::VirtualMachine::SetAddressReg},
        Instruction {&CHIP8::VirtualMachine::JumpWithOffset},
        Instruction {&CHIP8::VirtualMachine::AndWithRandom},
        Instruction {&CHIP8::VirtualMachine::Draw},
        Instruction {&CHIP8::VirtualMachine::SkipOnKeyState},
        Instruction {&CHIP8::VirtualMachine::FPrefixInstructions},
    };
}

void CHIP8::VirtualMachine::LoadProgram(std::span<const std::byte> program)
{
    std::ranges::copy(program, std::begin(m_memory) + INITIAL_ADDRESS);
}

void CHIP8::VirtualMachine::OnClock(const boost::system::error_code& errc)
{
    if (errc)
    {
        m_ioCtx.stop();
        return;
    }
    //fetch instruction
    const auto opcode = FetchNextInstruction();
    //decode it
    const auto decodedOpcode = DecodedOpcode{opcode};
    //find instruction
    const auto instruction = GetInstruction(decodedOpcode);
    //execute it
    instruction(this, std::cref(decodedOpcode));
    //increase value of program counter
    m_programCounter += INSTRUCTION_WIDTH;
    
    m_clock.expires_after(CLOCK_PERIOD);
    m_clock.async_wait(std::bind(&CHIP8::VirtualMachine::OnClock, this, std::placeholders::_1));
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

void CHIP8::VirtualMachine::DrawSprite(std::uint8_t x, std::uint8_t y, std::span<std::byte> sprite)
{
    const auto toBitsetAndReverse = 
    [](std::byte n)
    {
        auto bitset = std::bitset<8> {std::to_integer<std::uint8_t>(n)};
        for (const auto i : std::views::iota(0UZ, bitset.size() / 2))
        {
            const bool temp = bitset[i];
            bitset[i] = bitset[bitset.size() - i - 1];
            bitset[bitset.size() - i - 1] = temp;
        }
        return bitset;
    };
    bool erasedPixel {false};
    const std::uint8_t spriteRowsToDraw = (y + sprite.size()) < DISPLAY_HEIGHT ? sprite.size() : (DISPLAY_HEIGHT - y);
    for (const auto [spriteRowIndex, spriteRow] : 
        sprite | 
        std::views::take(spriteRowsToDraw) | 
        std::views::transform(toBitsetAndReverse) | 
        std::views::enumerate)
    {
        auto& currentDisplayRow = m_displayMemory.at(spriteRowIndex + y);
        const auto currentDisplayRowValue = currentDisplayRow.to_ullong();
        const std::uint64_t mask = 0xFFULL << x;
        const auto pixelsToOverwrite = currentDisplayRowValue & mask;
        if (pixelsToOverwrite > 0)
        {
            erasedPixel = true;
        }
        const auto newDisplayRow = currentDisplayRowValue ^ (spriteRow.to_ullong() << x);
        currentDisplayRow = newDisplayRow;
    }
    m_registers.at(0xF) = erasedPixel ? std::byte {1} : std::byte {0};
}

CHIP8::VirtualMachine::DisplayMemory CHIP8::VirtualMachine::GetDisplayMemory() const
{
    return m_displayMemory;
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
        //clear display
        case 0x0:
            m_displayMemory.fill(0);
        break;

        //return from subroutine
        case 0xE:
            m_programCounter = m_stack.back();
            m_stack.pop_back();
        break;

        //ignore anything else
        default:
            break;
    }
}

void CHIP8::VirtualMachine::Call(const DecodedOpcode& decodedOpcode)
{
    m_stack.push_back(m_programCounter);
    m_programCounter = decodedOpcode.GetAddress() - INSTRUCTION_WIDTH;
}

void CHIP8::VirtualMachine::Jump(const DecodedOpcode& decodedOpcode)
{
    std::uint16_t address = decodedOpcode.GetAddress();
    m_programCounter = address - INSTRUCTION_WIDTH;
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

void CHIP8::VirtualMachine::EightPrefixInstructions(const DecodedOpcode& decodedOpcode)
{
    const auto [firstRegIndex, secondRegIndex] = decodedOpcode.GetRegIndices();
    auto& firstReg = m_registers.at(firstRegIndex);
    auto& secondReg = m_registers.at(secondRegIndex);

    switch (decodedOpcode.nibbles.front()) 
    {
        //Vx = Vy
        case 0:
            firstReg = secondReg;
            break;

        //Vx = Vx OR Vy
        case 1:
            firstReg |= secondReg;
            break;

        //Vx = Vx AND Vy
        case 2:
            firstReg &= secondReg;
            break;

        //Vx = Vx XOR Vy
        case 3:
            firstReg ^= secondReg;
            break;

        //Vx = Vx + Vy, VF = 1 if overflow, 0 otherwise
        case 4:
        {
            auto result = std::to_integer<std::uint16_t>(firstReg);
            result += std::to_integer<std::uint16_t>(secondReg);
            firstReg = std::byte {static_cast<std::uint8_t>(result & 0xFF)};
            m_registers.at(0xF) = result > std::numeric_limits<std::uint8_t>::max() ? std::byte {1} : std::byte {0};
        }
            break;
        
        //Vx = Vx - Vy, VF = 1 if no borrow, 0 otherwise
        case 5:
        {
            auto result = std::to_integer<std::uint8_t>(firstReg);
            result -= std::to_integer<std::uint8_t>(secondReg);
            firstReg = std::byte {result};
            m_registers.at(0xF) = firstReg > secondReg ? std::byte {1} : std::byte {0};
        }
            break;

        //Vx = Vx >> 1, VF = least significant bit of Vx before shift
        case 6:
        {
            m_registers.at(0xF) = firstReg & std::byte {1};
            firstReg >>= 1;
        }
            break;

        //Vx = Vy - Vx, VF = 1 if no borrow, 0 otherwise
        case 7:
        {
            auto result = std::to_integer<std::uint8_t>(secondReg);
            result -= std::to_integer<std::uint8_t>(firstReg);
            firstReg = std::byte {result};
            m_registers.at(0xF) = secondReg > firstReg ? std::byte {1} : std::byte {0};
        }
            break;

        //Vx = Vx << 1, VF = most significant bit of Vx before shift
        case 0xE:
        {
            m_registers.at(0xF) = (firstReg & std::byte {0b1000'0000}) >> 7;
            firstReg <<= 1;
        }
            break;
    }
}

void CHIP8::VirtualMachine::SkipOnRegsNotEqual(const DecodedOpcode& decodedOpcode)
{
    const auto [firstRegIndex, secondRegIndex] = decodedOpcode.GetRegIndices();
    const auto& firstReg = m_registers.at(firstRegIndex);
    const auto& secondReg = m_registers.at(secondRegIndex);

    if (firstReg != secondReg)
    {
        SkipNextInstruction();
    }
}

void CHIP8::VirtualMachine::SetAddressReg(const DecodedOpcode& decodedOpcode)
{
    m_addressRegister = decodedOpcode.GetAddress();
}

void CHIP8::VirtualMachine::JumpWithOffset(const DecodedOpcode& decodedOpcode)
{
    m_programCounter = decodedOpcode.GetAddress() + std::to_integer<std::uint16_t>(m_registers.at(0));
}

void CHIP8::VirtualMachine::AndWithRandom(const DecodedOpcode& decodedOpcode)
{
    const auto randByte = std::byte {m_randomByteSrc()};
    const auto value = decodedOpcode.GetValue();
    const auto [regIndex, _] = decodedOpcode.GetRegIndices();
    m_registers.at(regIndex) = randByte & value;
}

void CHIP8::VirtualMachine::Draw(const DecodedOpcode& decodedOpcode)
{
    const auto [firstRegIndex, secondRegIndex] = decodedOpcode.GetRegIndices();
    const auto x = std::to_integer<std::uint8_t>(m_registers.at(firstRegIndex));
    const auto y = std::to_integer<std::uint8_t>(m_registers.at(secondRegIndex));
    const auto spriteSize = decodedOpcode.nibbles.front();
    const auto sprite = std::span {std::begin(m_memory) + m_addressRegister, spriteSize};
    DrawSprite(x, y, sprite);
}

void CHIP8::VirtualMachine::SkipOnKeyState(const DecodedOpcode& decodedOpcode)
{
    const auto [regIndex, _] = decodedOpcode.GetRegIndices();
    const auto operationCode = decodedOpcode.GetValue();
    const auto keyCode = std::to_integer<std::uint8_t>(m_registers.at(regIndex));
    
    if (operationCode == std::byte {0x9E} and m_keyboard.IsKeyPressed(CHIP8::Key{keyCode}))
    {
        SkipNextInstruction();
        return;
    }

    if (operationCode == std::byte {0xA1} and not m_keyboard.IsKeyPressed(CHIP8::Key{keyCode}))
    {
        SkipNextInstruction();
        return;
    }
}

void CHIP8::VirtualMachine::FPrefixInstructions(const DecodedOpcode& decodedOpcode)
{
    const auto [regIndex, _] = decodedOpcode.GetRegIndices();
    const auto operationCode = std::to_integer<std::uint8_t>(decodedOpcode.GetValue());

    switch(operationCode)
    {
        //Vx = delay timer
        case 0x07:
            m_registers.at(regIndex) = std::byte{m_delayTimer.GetValue()};
            break;

        //wait for a key to be pressed and store the key code in Vx 
        case 0x0A:
        {
            const auto pressedKey = static_cast<std::uint8_t>(m_keyboard.WaitForKeyPress());
            m_registers.at(regIndex) = std::byte{pressedKey};
        }
            break;
        
        //delay timer = Vx
        case 0x15:
            m_delayTimer.Set(std::to_integer<std::uint8_t>(m_registers.at(regIndex)));
            break;
        
        //sound timer = Vx
        case 0x18:
            m_soundTimer.Set(std::to_integer<std::uint8_t>(m_registers.at(regIndex)));
            break;

        //I = I + Vx
        case 0x1E:
            m_addressRegister += std::to_integer<std::uint16_t>(m_registers.at(regIndex));
            break;

        //I = memory location of digit Vx
        case 0x29:
        {
            const auto digit = std::to_integer<std::uint16_t>(m_registers.at(regIndex));
            const auto digitAddressStart = FONT_ADDRESS_START + digit * HEX_DIGIT_SPRITE_SIZE;
            m_addressRegister = digitAddressStart;
        }
            break;

        //store BCD of Vx in memory
        case 0x33:
        {
            const auto bcd = ToBCD(std::to_integer<std::uint8_t>(m_registers.at(regIndex)));
            std::ranges::copy(bcd, std::begin(m_memory) + m_addressRegister);
        }
            break;
        
        //store registers from 0 to x in memory
        case 0x55:
            std::copy(std::begin(m_registers), std::begin(m_registers) + regIndex + 1, 
                std::begin(m_memory) + m_addressRegister);
            break;
        
        //read registers from 0 to x from memory
        case 0x65:
            std::copy(std::begin(m_memory) + m_addressRegister, std::begin(m_memory) + m_addressRegister + regIndex + 1, 
                std::begin(m_registers));
            break;
    }
}

#pragma endregion Instructions