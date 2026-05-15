cmake_minimum_required(VERSION 3.15)

project(STM32f407vg_signal_generator LANGUAGES C CXX ASM)

find_bsp(ID com.sysprogs.project_importers.stm32.cubemx SOURCE_PROJECT STM32f407vg_signal_generator.ioc)

add_bsp_based_executable(NAME STM32f407vg_signal_generator
	GENERATE_BIN
	GENERATE_MAP)

target_sources(${PROJECT_NAME} PRIVATE
	Core/Src/encoder.c
	Core/Src/signal_generator.c
	Core/Src/tft_st7735.c
	Core/Src/ui.c
	Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c)

target_include_directories(${PROJECT_NAME} PRIVATE
	Core/Inc)
