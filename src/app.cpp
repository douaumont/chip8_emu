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

#include <cstddef>
#include <cstdlib>
#include <ostream>
#include <vector>
#include <print>
#include <ranges>
#include <thread>
#include <fstream>
#include <filesystem>
#include <optional>
#include <filesystem>
#include <iostream>
#include <boost/program_options.hpp>
#include <SFML/Graphics.hpp>
#include "chip8/chip8vm.hpp"
#include "app.hpp"

Emulator::Emulator(int argc, char** argv)
{
    namespace po = boost::program_options;

    po::options_description desc("Arguments");
    desc.add_options()
        ("help", "Show this message")
        ("program-file,p", po::value<std::string>()->required(), "Path to file with CHIP-8 program");
    
    po::variables_map options;
    po::store(po::parse_command_line(argc, argv, desc), options);
    po::notify(options);

    if (options.count("help"))
    {
        std::cout << desc << '\n';
        std::exit(EXIT_SUCCESS);
    }
    
    const auto pathToProgramFile {options.at("program-file").as<std::string>()};
    std::vector<std::byte> program;
    {
        std::ifstream programFile {pathToProgramFile, std::ios::in | std::ios::binary};
        if (programFile.is_open())
        {
            program.resize(std::filesystem::file_size(pathToProgramFile));
            programFile.read(reinterpret_cast<char*>(program.data()), program.size());
        }
        else  
        {
            std::println("Cannot open file {}!", pathToProgramFile);
            std::exit(EXIT_FAILURE);
        }
    }

    m_virtualMachine.LoadProgram(program);
}

void Emulator::Run()
{
    std::jthread vmThread {&CHIP8::VirtualMachine::Run, &m_virtualMachine};
    sf::RenderWindow mainWindow {sf::VideoMode{640, 320}, "CHIP-8 emulator"};
    
    sf::Image whiteRectImage;
    whiteRectImage.create(10, 10, sf::Color::White);
    sf::Texture whiteRectTexture;
    whiteRectTexture.loadFromImage(whiteRectImage);

    CHIP8::VirtualMachine::DisplayMemory previousDisplay {boost::extents[m_virtualMachine.GetDisplayHeight()][m_virtualMachine.GetDisplayWidth()]};

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

        auto display = m_virtualMachine.GetDisplayMemory();
        if (display.has_value())
        {
            previousDisplay = display.value();
        }
        else
        {
            display = previousDisplay;
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

    m_virtualMachine.Stop();
}