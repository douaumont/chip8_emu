#pragma once

#include <array>
#include <SFML/Window/Keyboard.hpp>

namespace CHIP8
{
    enum class Key
    {
        _1 = 0, _2, _3, C,
        _4, _5, _6, D,
        _7, _8, _9, E,
        A, _0, B, F
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