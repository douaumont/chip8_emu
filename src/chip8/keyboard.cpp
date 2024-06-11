#include "keyboard.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <utility>
#include <ranges>

CHIP8::Keyboard::Keyboard()
    :
    m_chip8KeyToPhysicalKey
    {
        sf::Keyboard::Key::X,
        sf::Keyboard::Key::Num1,
        sf::Keyboard::Key::Num2,
        sf::Keyboard::Key::Num3,
        sf::Keyboard::Key::Q,
        sf::Keyboard::Key::W,
        sf::Keyboard::Key::E,
        sf::Keyboard::Key::A,
        sf::Keyboard::Key::S,
        sf::Keyboard::Key::D,
        sf::Keyboard::Key::Z,
        sf::Keyboard::Key::C,
        sf::Keyboard::Key::Num4,
        sf::Keyboard::Key::R,
        sf::Keyboard::Key::F,
        sf::Keyboard::Key::V,
    }
{

}

bool CHIP8::Keyboard::IsKeyPressed(CHIP8::Key key) const
{
    return sf::Keyboard::isKeyPressed(m_chip8KeyToPhysicalKey.at(std::to_underlying(key)));
}

CHIP8::Key CHIP8::Keyboard::WaitForKeyPress() const
{
    while (true)
    {
        for (const auto [chip8Key, physicalKey] : m_chip8KeyToPhysicalKey | std::views::enumerate)
        {
            if (sf::Keyboard::isKeyPressed(physicalKey))
            {
                return CHIP8::Key {static_cast<int>(chip8Key)};
            }
        }
    }
}