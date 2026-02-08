cmake_minimum_required(VERSION 3.13)

# --- DOWNLOAD PICO SDK IF NOT FOUND ---
include(FetchContent)

FetchContent_Declare(
	pico_sdk_fetch
	GIT_REPOSITORY         https://github.com/raspberrypi/pico-sdk
	GIT_TAG                2.2.0
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

project(my_console C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# --- SOURCE ---
add_executable(my_console
	src/drivers/graphics/lcd.c
	src/drivers/graphics/os.c
	src/drivers/buttons.c
	src/drivers/pins.c
	src/drivers/sd_card.c
	src/main.c
)

# --- LTO ---
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if (result)
	set_property(TARGET my_console PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	message(STATUS "LTO (Link Time Optimization) Enabled")
else()
	message(WARNING "LTO not supported: ${output}")
endif()

# --- STRIP DEAD CODE ---
target_compile_options(my_console PRIVATE
	-ffunction-sections
	-fdata-sections
)
target_link_options(my_console PRIVATE
	-Wl,--gc-sections
	# for GCC 13 LTO - force linker to keep the IO wrappers
	-Wl,--undefined=__wrap_printf
	-Wl,--undefined=__wrap_vprintf
	-Wl,--undefined=__wrap_snprintf
	-Wl,--undefined=__wrap_vsnprintf
	-Wl,--undefined=__wrap_puts
	-Wl,--undefined=__wrap_putchar
	-Wl,--undefined=__wrap_getchar
)

# --- DISABLE EXCEPTIONS AND RTTI
target_compile_options(my_console PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
	$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

# --- DEBUG CONFIG ---
# generate .map files to see memory on crash in debug mode
target_link_options(my_console PRIVATE
	$<$<CONFIG:Debug>:-Wl,-Map=${CMAKE_BINARY_DIR}/my_console.map>
)
# don't use O0, optimise for debugging
target_compile_options(my_console PRIVATE
	$<$<CONFIG:Debug>:-Og>
	$<$<CONFIG:Debug>:-g3>
)

# --- RELEASE CONFIG ---
target_compile_options(my_console PRIVATE
	$<$<CONFIG:Release>:-O3>
)

# --- PICO RUNTIME OPTIMISATIONS ---
# "pico" (balanced) | "compiler" (pico + all C99 formatting flags, huge) | "minimal" (no floats of 64-bit integers and limited formatting, tiny)
pico_set_printf_implementation(my_console pico)

# use hardware FPU for fast math
pico_set_float_implementation(my_console pico)

# --- INCLUDE ---
target_include_directories(my_console PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --- LIBRARIES ---
target_link_libraries(my_console
	pico_stdlib
	hardware_spi
	hardware_dma
	hardware_pio
	hardware_interp
)

# --- USB OUTPUT FIX ---
# Keeps the USB alive for serial debugging
#if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
#	pico_enable_stdio_usb(my_console 0)
#else()
	pico_enable_stdio_usb(my_console 1)
#endif()

pico_enable_stdio_uart(my_console 0)

pico_add_extra_outputs(my_console)
