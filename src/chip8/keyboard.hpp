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
#include <SFML/Window/Keyboard.hpp>

namespace CHIP8
{
    enum class Key
    {
        _0 = 0, _1, _2, _3,
        _4, _5, _6, _7,
        _8, _9, A, B, 
        C, D, E, F
    };

    class Keyboard
    {
        static constexpr unsigned KEYS = 16;
        std::array<sf::Keyboard::Key, KEYS> m_chip8KeyToPhysicalKey;
        
    public:
        Keyboard();
        bool IsKeyPressed(CHIP8::Key key) const;
        CHIP8::Key WaitForKeyPress() const;
    };
}