#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Clock.hpp>
#include <algorithm>
#include <cstddef>
#include <exception>
#include <ostream>
#include <vector>
#include <print>
#include <ranges>
#include <thread>
#include <fstream>
#include <filesystem>
#include <string_view>
#include <optional>
#include <SFML/Graphics.hpp>
#include "chip8/chip8vm.hpp"

using namespace std::string_view_literals;

int main()
{
    std::vector<std::byte> testProgram;
    {
        constexpr auto FILE = "Pong (1 player).ch8"sv;
        std::ifstream testProgramFile {FILE.data(), std::ios::in | std::ios::binary};
        if (testProgramFile.is_open())
        {
            testProgram.resize(std::filesystem::file_size(FILE));
            testProgramFile.read(reinterpret_cast<char*>(testProgram.data()), testProgram.size());
        }
    }
    CHIP8::VirtualMachine vm;
    vm.LoadProgram(testProgram);
    std::jthread vmThread {&CHIP8::VirtualMachine::Run, &vm};
    sf::RenderWindow mainWindow {sf::VideoMode{640, 320}, "CHIP-8 emulator"};
    
    sf::Image whiteRectImage;
    whiteRectImage.create(10, 10, sf::Color::White);
    sf::Texture whiteRectTexture;
    whiteRectTexture.loadFromImage(whiteRectImage);

    std::optional<CHIP8::VirtualMachine::DisplayMemory> previousDisplay;

    while (mainWindow.isOpen())
    {
        sf::Event event;
        while (mainWindow.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            { 
                mainWindow.close();
            }
        }
        
        mainWindow.clear(sf::Color::Black);

        auto display = vm.GetDisplayMemory();
        if (display.has_value())
        {
            previousDisplay = display;
        }
        else if (previousDisplay.has_value())
        {
            display = previousDisplay;
        }
        else
        {
            continue;
        }

        for (const auto [rowIndex, row] : display.value() | std::views::enumerate)
        {
            for (const auto columnIndex : std::views::iota(0UZ, row.size()))
            {
                const auto pixel = row[columnIndex];
                if (pixel)
                {
                    sf::Sprite whiteRect {whiteRectTexture};
                    whiteRect.setPosition(columnIndex * 10, rowIndex * 10);
                    mainWindow.draw(whiteRect);
                }
            }
        }

        mainWindow.display();
    }

    vm.Stop();
    
    return 0;
}