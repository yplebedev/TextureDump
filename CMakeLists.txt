cmake_minimum_required(VERSION 3.31.6)
project(TextureDump)
set(CMAKE_CXX_STANDARD 26)

if (DEFINED ENV{RESHADE_PATH})
    include_directories($ENV{RESHADE_PATH}/include)
    include_directories($ENV{RESHADE_PATH}/deps/imgui)
else()
    message(SEND_ERROR "No $RESHADE_PATH variable set! Please set it and try again.")
endif()

include_directories(lib/tinyexr/deps/miniz)
include_directories(lib/tinyexr)
include_directories(lib/stb)

add_library(TextureDump SHARED main.cpp
        lib/tinyexr/deps/miniz/miniz.c
        NamedTexture.h
        SaveData.h
        ISavingStrategy.h
        SavingStrategiesImpl.h
        Utils.h
        CallbackFunctions.h)

set_target_properties(TextureDump PROPERTIES SUFFIX .addon64)