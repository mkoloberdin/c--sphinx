cmake_minimum_required(VERSION 3.10)
project(c__sphinx)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

find_package (Boost REQUIRED COMPONENTS filesystem system)
include_directories(${Boost_INCLUDE_DIR})

set(SOURCE_FILES
        src/asm.h
        src/asm_name.h
        src/asmnemon.h
        src/class.cpp
        src/coff.h
        src/const.h
        src/debug.cpp
        src/dirlist.h
        src/disasm.cpp
        src/errors.cpp
        src/id.h
        src/le.h
        src/libobj.cpp
        src/main.cpp
        src/misc.cpp
        src/new_type.cpp
        src/optreg.cpp
        src/outle.cpp
        src/outobj.cpp
        src/outpe.cpp
        src/pointer.cpp
        src/struct.h
        src/switch.cpp
        src/table.h
        src/tok.h
        src/toka.cpp
        src/tokb.cpp
        src/tokc.cpp
        src/toke.cpp
        src/tokr.cpp
        src/res.cpp
        src/res.h
        src/resname.h)

add_executable(c-- ${SOURCE_FILES})

target_link_libraries(c-- ${Boost_LIBRARIES})
