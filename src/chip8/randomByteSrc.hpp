#pragma once

#include <cstdint>
#include <random>

namespace CHIP8
{
    class RandomByteSource
    {
        std::minstd_rand m_engine;
        std::uniform_int_distribution<std::uint8_t> m_distr;

    public:
        RandomByteSource();
        std::uint8_t operator()();
    };
}