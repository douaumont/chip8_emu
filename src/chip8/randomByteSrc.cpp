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

#include "randomByteSrc.hpp"
#include <random>

CHIP8::RandomByteSource::RandomByteSource()
    :
    m_engine(std::random_device{}())
{

}

std::uint8_t CHIP8::RandomByteSource::operator()()
{
    return m_distr(m_engine);
}