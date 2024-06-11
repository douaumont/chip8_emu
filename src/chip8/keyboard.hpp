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