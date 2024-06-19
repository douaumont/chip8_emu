/*
    Copyright 2024 Artyom Makarov

    This file is part of chip8_emu.

    chip8_emu is free software: you can redistribute it and/or modify it under the terms of the 
    GNU General Public License as published by the Free Software Foundation, 
    either version 3 of the License, or (at your option) any later version.

    chip8_emu is distributed in the hope that it will be useful, 
    but WITHOUT ANY WARRANTY; 
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with chip8_emu. 
    If not, see <https://www.gnu.org/licenses/>. 
*/

#pragma once

#include <array>
#include <bit>
#include <functional>
#include <span>
#include <mutex>
#include <optional>
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <boost/container/static_vector.hpp>
#include <boost/asio.hpp>
#include <boost/multi_array.hpp>
#include "timer.hpp"
#include "randomByteSrc.hpp"
#include "keyboard.hpp"

namespace CHIP8
{
    namespace asio = boost::asio;

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
    public:
        using DisplayMemory = boost::multi_array<bool, 2>;
        enum class State 
        {
            Running,
            Shutdown
        };
        
    private:

        static constexpr unsigned int 
            MEMORY_SIZE = 4096, 
            REGISTER_COUNT = 16,
            DISPLAY_WIDTH = 64,
            DISPLAY_HEIGHT = 32,
            ADDRESS_BUS_WIDTH = std::bit_width(MEMORY_SIZE),
            INITIAL_ADDRESS = 0x200,
            INSTRUCTION_WIDTH = 2,
            STACK_SIZE = 16,
            HEX_DIGIT_SPRITE_SIZE = 5,
            FONT_SIZE = HEX_DIGIT_SPRITE_SIZE * 16,
            FONT_ADDRESS_START = 0x50;

        static constexpr auto CLOCK_PERIOD = 2ms;

        static constexpr std::array<std::uint8_t, FONT_SIZE> FONT = 
        {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };
        
        using Instruction = std::function<void(VirtualMachine* vm, const DecodedOpcode&)>;

        std::array<std::byte, MEMORY_SIZE> m_memory;
        std::array<std::byte, REGISTER_COUNT> m_registers;
        std::uint16_t m_addressRegister, m_programCounter;
        boost::container::static_vector<std::uint16_t, STACK_SIZE> m_stack;

        std::mutex m_displayMemoryMtx;
        DisplayMemory m_displayMemory;
        
        RandomByteSource m_randomByteSrc;
        Keyboard m_keyboard;

        asio::io_context m_ioCtx;
        Timer m_delayTimer, m_soundTimer;
        asio::steady_timer m_clock;
        std::atomic<State> m_state;

        std::array<Instruction, 16> m_instructionTable;

        std::uint16_t FetchNextInstruction() const;
        Instruction GetInstruction(const DecodedOpcode& decodedOpcode) const;

        void DrawSprite(std::uint8_t x, std::uint8_t y, std::span<std::byte> sprite);
        void ClearDisplay();

        /*Instructions*/ 

        void UnimplementedInstruction(const DecodedOpcode& decodedOpcode);

        //prefix = 0
        void ZeroPrefixInstuctions(const DecodedOpcode& decodedOpcode);

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

        //prefix = C
        void AndWithRandom(const DecodedOpcode& decodedOpcode);

        //prefix = D
        void Draw(const DecodedOpcode& decodedOpcode);

        //prefix = E
        void SkipOnKeyState(const DecodedOpcode& decodedOpcode);

        //prefix = F
        void FPrefixInstructions(const DecodedOpcode& decodedOpcode);

        void OnClock(const boost::system::error_code& errc);

    public:
        VirtualMachine();
        void LoadProgram(std::span<const std::byte> program);
        std::optional<DisplayMemory> GetDisplayMemory();
        void Stop();
        void Run();
        unsigned int GetDisplayHeight() const;
        unsigned int GetDisplayWidth() const;
    };
}