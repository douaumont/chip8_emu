cmake_minimum_required(VERSION 3.25)
project(chip8_emu LANGUAGES CXX)

find_package(SFML 2.6.1 REQUIRED COMPONENTS graphics window system)
find_package(Boost 1.32 REQUIRED program_options nowide)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    src/chip8/chip8vm.cpp
    src/chip8/randomByteSrc.cpp
    src/chip8/keyboard.cpp
    src/chip8/timer.cpp
    src/app.cpp
    src/main.cpp)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_link_libraries(${PROJECT_NAME} PRIVATE sfml-graphics sfml-window sfml-system ${Boost_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})