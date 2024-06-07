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