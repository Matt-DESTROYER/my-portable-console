cmake_minimum_required(VERSION 3.13)

include(FetchContent)

FetchContent_Declare(
	pico_sdk_fetch
	GIT_REPOSITORY         https://github.com/raspberrypi/pico-sdk
	GIT_TAG                master
	GIT_SHALLOW            TRUE
	GIT_SUBMODULES_RECURSE TRUE
	SOURCE_DIR             "${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk"
)

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk")
	message(STATUS "Downloading Pico SDK...")

	FetchContent_GetProperties(pico_sdk_fetch)

	if (NOT pico_sdk_fetch_POPULATED)
		if(POLICY CMP0169)
			cmake_policy(SET CMP0169 OLD)
		endif()

		FetchContent_Populate(pico_sdk_fetch)
	endif()
endif()

set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk")

set(PICO_BOARD pico2)
set(PICO_PLATFORM rp2350-arm-s)

include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(my_console C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# --- SOURCE ---
add_executable(my_console
	src/main.c
	src/drivers/screen.c
	src/drivers/sd_card.c
	src/drivers/buttons.c
)

# --- INCLUDE ---
target_include_directories(my_console PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --- LIBRARIES ---
target_link_libraries(my_console pico_stdlib hardware_spi)

# --- USB OUTPUT FIX ---
# Keeps the USB alive for serial debugging
pico_enable_stdio_usb(my_console 1)
pico_enable_stdio_uart(my_console 0)

pico_add_extra_outputs(my_console)
