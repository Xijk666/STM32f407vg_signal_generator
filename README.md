# STM32f407vg_signal_generator

<p align="center">
  <img src="https://img.shields.io/badge/MCU-STM32F407VGT6-03234B?style=for-the-badge&logo=stmicroelectronics&logoColor=white" alt="MCU">
  <img src="https://img.shields.io/badge/IDE-VisualGDB-5C2D91?style=for-the-badge&logo=visualstudio&logoColor=white" alt="VisualGDB">
  <img src="https://img.shields.io/badge/CubeMX-ready-00A6A6?style=for-the-badge" alt="CubeMX">
  <img src="https://img.shields.io/badge/DMA-enabled-FF8C00?style=for-the-badge" alt="DMA">
</p>

<p align="center">
  <a href="https://github.com/doubthek1ng2007/STM32f407vg_signal_generator/stargazers">
    <img src="https://img.shields.io/github/stars/doubthek1ng2007/STM32f407vg_signal_generator?style=for-the-badge&logo=github&label=Stars" alt="GitHub stars">
  </a>
  <a href="https://github.com/doubthek1ng2007/STM32f407vg_signal_generator/network/members">
    <img src="https://img.shields.io/github/forks/doubthek1ng2007/STM32f407vg_signal_generator?style=for-the-badge&logo=github&label=Forks" alt="GitHub forks">
  </a>
  <a href="https://github.com/doubthek1ng2007/STM32f407vg_signal_generator">
    <img src="https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fgithub.com%2Fdoubthek1ng2007%2FSTM32f407vg_signal_generator&count_bg=%2300A6A6&title_bg=%23222222&icon=github.svg&icon_color=%23FFFFFF&title=Views&edge_flat=false" alt="Repository views">
  </a>
  <img src="https://img.shields.io/github/last-commit/doubthek1ng2007/STM32f407vg_signal_generator?style=for-the-badge&label=Last%20commit" alt="Last commit">
</p>

<p align="center">
  <a href="#russian"><b>Русский</b></a>
  &nbsp;|&nbsp;
  <a href="#english"><b>English</b></a>
</p>

---

<h2 id="russian">Русский</h2>

Прошивка генератора сигналов на базе **STM32F407VGT6**. Проект подготовлен для **Visual Studio + VisualGDB** и **STM32CubeMX**, использует аппаратные таймеры, DMA, TFT-дисплей ST7735 128x160 и энкодер с антидребезгом.

### Возможности

- Генерация сигналов через TIM1 и TIM8.
- Два основных канала для осциллографа: CH1 и CH2.
- Прямые и инверсные выходы для проверки сигналов.
- Обновление TFT-дисплея через SPI + DMA.
- Интерфейс на дисплее ST7735 128x160 RGB.
- Энкодер с обработкой дребезга, коротким нажатием и долгим нажатием.
- Выбор шага энкодера для частоты: `1`, `5`, `10`, `50`, `100`, `250`, `500`, `1000`, `10000` Гц.
- Подпись `By Xijk` на экране устройства.

### Подключение

| Модуль | Назначение |
| --- | --- |
| MCU | STM32F407VGT6 |
| Дисплей | 1.8 inch TFT LCD, 128x160, ST7735-compatible RGB |
| Энкодер | 5V, GND, KEY, S1, S2 |
| Осциллограф CH1 | TIM1, прямой и инверсный выход |
| Осциллограф CH2 | TIM8, прямой и инверсный выход |

Полная схема подключения находится в [`HARDWARE_CONNECTIONS.md`](STM32f407vg_signal_generator/HARDWARE_CONNECTIONS.md).

### Сборка

Открой `STM32f407vg_signal_generator.sln` в Visual Studio с установленным VisualGDB и собери конфигурацию Debug.

```powershell
$env:TOOLCHAIN_ROOT='C:\SysGCC\arm-eabi'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build STM32f407vg_signal_generator\build\VisualGDB\Debug --config Debug
```

### Управление

| Действие энкодера | Результат |
| --- | --- |
| Вращение на `FREQ` | Изменение частоты выбранным шагом `STEP` |
| Вращение на `WIDTH` | Изменение ширины импульса |
| Вращение на `PHASE` | Изменение фазы |
| Вращение на `STEP` | Выбор шага энкодера |
| Короткое нажатие | Переход к следующему пункту |
| Долгое нажатие | Переключение режима отображения выбранного пункта |

---

<h2 id="english">English</h2>

Firmware for a two-channel signal generator based on **STM32F407VGT6**. The project is prepared for **Visual Studio + VisualGDB** and **STM32CubeMX**, using hardware timers, DMA, a 128x160 ST7735 TFT display, and a debounced rotary encoder UI.

### Features

- TIM1 and TIM8 based signal generation.
- Two main oscilloscope channels: CH1 and CH2.
- Direct and inverted outputs for signal verification.
- TFT display update over SPI + DMA.
- ST7735 128x160 RGB display UI.
- Rotary encoder with debounce, click and long-press handling.
- Selectable encoder frequency step: `1`, `5`, `10`, `50`, `100`, `250`, `500`, `1000`, `10000` Hz.
- `By Xijk` signature on the device screen.

### Hardware

| Module | Purpose |
| --- | --- |
| MCU | STM32F407VGT6 |
| Display | 1.8 inch TFT LCD, 128x160, ST7735-compatible RGB |
| Encoder | 5V, GND, KEY, S1, S2 |
| Oscilloscope CH1 | TIM1 direct and inverted output |
| Oscilloscope CH2 | TIM8 direct and inverted output |

Full wiring notes are available in [`HARDWARE_CONNECTIONS.md`](STM32f407vg_signal_generator/HARDWARE_CONNECTIONS.md).

### Build

Open `STM32f407vg_signal_generator.sln` in Visual Studio with VisualGDB installed, then build the Debug configuration.

```powershell
$env:TOOLCHAIN_ROOT='C:\SysGCC\arm-eabi'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build STM32f407vg_signal_generator\build\VisualGDB\Debug --config Debug
```

### UI Controls

| Encoder action | Result |
| --- | --- |
| Rotate on `FREQ` | Change frequency by selected `STEP` |
| Rotate on `WIDTH` | Change pulse width |
| Rotate on `PHASE` | Change phase |
| Rotate on `STEP` | Select encoder step |
| Short press | Move to next field |
| Long press | Toggle display mode for selected field |

---

## Project Layout

```text
STM32f407vg_signal_generator/
|-- STM32f407vg_signal_generator.sln
`-- STM32f407vg_signal_generator/
    |-- Core/
    |   |-- Inc/
    |   |-- Src/
    |   `-- Startup/
    |-- Drivers/
    |-- CMakeLists.txt
    |-- HARDWARE_CONNECTIONS.md
    |-- STM32f407vg_signal_generator.ioc
    `-- STM32f407vg_signal_generator.vgdbcmake
```

## Display Offset

The ST7735 visible RAM offset is configured in `Core/Inc/tft_st7735.h`:

```c
#define TFT_COL_OFFSET 2U
#define TFT_ROW_OFFSET 1U
```

If your exact TFT module shifts the image differently, tune only these two values.

## License

This repository includes STM32 HAL/CMSIS files under their original ST licenses in the `Drivers/` directory. Project-specific source files are intended for educational and lab work.

