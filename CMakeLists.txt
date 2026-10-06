cmake_minimum_required(VERSION 3.20)

# CLion uses the same cross compiler and metadata as the actual PlatformIO build.
file(TO_CMAKE_PATH "$ENV{USERPROFILE}/.platformio" PIO_HOME)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_C_COMPILER "${PIO_HOME}/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-gcc.exe")
set(CMAKE_CXX_COMPILER "${PIO_HOME}/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-g++.exe")
project(AeroPad LANGUAGES C CXX)

set(PIO_PYTHON "${PIO_HOME}/penv/Scripts/python.exe")
execute_process(
    COMMAND "${PIO_PYTHON}" "${CMAKE_SOURCE_DIR}/tools/clion_metadata.py" "${CMAKE_BINARY_DIR}/pio-index.cmake"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE metadata_status
)
if(NOT metadata_status EQUAL 0)
    message(FATAL_ERROR "PlatformIO metadata failed. Run build.cmd and reload CMake.")
endif()
include("${CMAKE_BINARY_DIR}/pio-index.cmake")
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h include/*.h)
add_library(code_index OBJECT EXCLUDE_FROM_ALL ${APP_SOURCES})
target_include_directories(code_index PRIVATE ${PIO_INCLUDES})
target_compile_definitions(code_index PRIVATE ${PIO_DEFINES})
target_compile_options(code_index PRIVATE ${PIO_CXX_FLAGS})

function(pio_target target_name)
    add_custom_target(${target_name}
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONUTF8=1 PYTHONIOENCODING=utf-8
            "${PIO_PYTHON}" -m platformio ${ARGN}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        USES_TERMINAL VERBATIM
    )
endfunction()
pio_target(firmware run -e aeropad)
add_custom_target(default_build ALL DEPENDS firmware)
pio_target(upload run -e aeropad -t upload)
pio_target(serial_monitor device monitor -p COM11 -b 115200)
pio_target(compilation_database run -e aeropad -t compiledb)
pio_target(clean_firmware run -e aeropad -t clean)
