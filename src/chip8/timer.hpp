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