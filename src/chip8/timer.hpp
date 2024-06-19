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

#include <atomic>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

namespace CHIP8
{
    namespace asio = boost::asio;

    using namespace std::chrono_literals;

    class Timer 
    {
        static constexpr auto PERIOD = 17ms;
        std::atomic_uint8_t m_value;
        asio::steady_timer m_sixtyHerzTimer;

        void SetTimer();
        void OnTimer(const boost::system::error_code& errc);

    public:
        Timer(asio::io_context& ctx);
        ~Timer();

        void Set(std::uint8_t value);
        std::uint8_t GetValue() const;
    };
}