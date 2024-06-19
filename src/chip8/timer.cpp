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

#include <boost/asio/error.hpp>
#include <functional>
#include "timer.hpp"

CHIP8::Timer::Timer(asio::io_context& ctx)
    :
    m_value(0),
    m_sixtyHerzTimer(ctx)
{
    
}

CHIP8::Timer::~Timer()
{
    m_sixtyHerzTimer.cancel();
}

void CHIP8::Timer::Set(std::uint8_t value)
{
    if (m_value == 0)
    {
        m_value = value;
        SetTimer();
    }
}

void CHIP8::Timer::OnTimer(const boost::system::error_code& errc)
{
    if (m_value > 0 and not errc)
    {
        m_value -= 1;
        SetTimer();
    }
}

void CHIP8::Timer::SetTimer()
{
    m_sixtyHerzTimer.expires_after(PERIOD);
    m_sixtyHerzTimer.async_wait(std::bind(&CHIP8::Timer::OnTimer, this, std::placeholders::_1));
}

std::uint8_t CHIP8::Timer::GetValue() const
{
    return m_value;
}