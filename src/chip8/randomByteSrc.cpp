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