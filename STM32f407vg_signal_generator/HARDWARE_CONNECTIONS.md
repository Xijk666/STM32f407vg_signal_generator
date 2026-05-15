# Полная схема подключения STM32F407VG Signal Generator

Проект: `STM32f407vg_signal_generator`  
Контроллер: `STM32F407VGT6`, корпус `LQFP100`, логические уровни GPIO `3.3 В`.

Проверено по datasheet `C:\Users\Xijk\Downloads\dm00037051.pdf`:

- стр. 44: LQFP100 pinout содержит PE8, PE9, PC6, PA7, PB10-PB15;
- стр. 52: PA7 поддерживает `TIM8_CH1N` и `TIM1_CH1N`;
- стр. 53: PE8 = `TIM1_CH1N`, PE9 = `TIM1_CH1`;
- стр. 55: PB12/PB13/PB14/PB15 поддерживают SPI2;
- стр. 66: PC6 поддерживает `TIM8_CH1`;
- стр. 68: alternate-function table подтверждает PE8/PE9 для TIM1.

## 1. Питание и общая земля

| Узел | Подключение | Примечание |
|---|---|---|
| STM32 `3V3` | стабилизированные 3.3 В | Все GPIO работают от 3.3 В |
| STM32 `GND` | общая земля | Соединить с GND дисплея, энкодера, осциллографа |
| TFT `VCC` | 3.3 В | Не 5 В, если модуль без стабилизатора и level-shifter |
| TFT `GND` | GND | Общая земля |
| Encoder `VCC/5V` | 3.3 В | Вывод может называться 5V, но на GPIO STM32 нельзя подавать 5 В |
| Encoder `GND` | GND | Общая земля |

Важно: если конкретный модуль энкодера реально требует питание 5 В, на линии `S1`, `S2`, `KEY` нужно поставить согласование уровней до 3.3 В.

## 2. Выходы генератора и осциллограф

| Сигнал | STM32F407VGT6 | Таймер | AF | Назначение | Рекомендация |
|---|---:|---|---:|---|---|
| `OUT_A` | PE9 | TIM1_CH1 | AF1 | прямой выход пары A | через 100-330 Ом к BNC/щупу |
| `OUT_A_INV` | PE8 | TIM1_CH1N | AF1 | инверсный выход пары A | через 100-330 Ом к BNC/щупу |
| `OUT_B` | PC6 | TIM8_CH1 | AF3 | прямой выход пары B | через 100-330 Ом к BNC/щупу |
| `OUT_B_INV` | PA7 | TIM8_CH1N | AF3 | инверсный выход пары B | через 100-330 Ом к BNC/щупу |
| `GND` | GND | - | - | общий провод | к земле осциллографа |

Для твоего двухканального осциллографа основной режим проверки:

| Канал осциллографа | Что подключать |
|---|---|
| CH1 | `OUT_A` или `OUT_B`, прямой сигнал |
| CH2 | `OUT_A_INV` или `OUT_B_INV`, инверсный сигнал |
| GND щупов | только к общей земле схемы |

Инверсия сделана аппаратно через `CH1N`, программного переключения GPIO в критическом цикле нет.

## 3. TFT LCD 1.8" 128x160 RGB, ST7735, SPI2

| Вывод дисплея | STM32F407VGT6 | Режим CubeMX/код | Примечание |
|---|---:|---|---|
| `VCC` | 3V3 | питание | 3.3 В |
| `GND` | GND | земля | общая земля |
| `SCK` / `SCL` | PB13 | SPI2_SCK, AF5 | тактирование SPI |
| `SDA` / `MOSI` | PB15 | SPI2_MOSI, AF5 | данные к дисплею |
| `CS` | PB12 | GPIO_Output | выбор дисплея |
| `DC` / `A0` | PB14 | GPIO_Output | команда/данные |
| `RST` / `RES` | PB10 | GPIO_Output | сброс дисплея |
| `BL` / `LED` | PB11 | GPIO_Output | подсветка, высокий уровень включает |

`MISO` не используется. SPI2 настроен как master transmit-only simplex.

## 4. Энкодер

| Вывод энкодера | STM32F407VGT6 | Режим | Обработка |
|---|---:|---|---|
| `VCC` / `5V` | 3V3 | питание | не подавать 5 В на входы STM32 |
| `GND` | GND | земля | общая земля |
| `S1` / `CLK` | PA0 | GPIO_Input + Pull-Up | антидребезг 2 ms |
| `S2` / `DT` | PA1 | GPIO_Input + Pull-Up | антидребезг 2 ms |
| `KEY` / `SW` | PA2 | GPIO_Input + Pull-Up | антидребезг 25 ms, long press 700 ms |

Логика кнопки и S1/S2 активная низким уровнем: при замыкании на GND читается `0`, в коде это считается нажатием/событием.

## 5. SWD для прошивки и отладки

| ST-LINK / программатор | STM32F407VGT6 | Примечание |
|---|---:|---|
| `SWDIO` | PA13 | стандартный SWD |
| `SWCLK` | PA14 | стандартный SWD |
| `GND` | GND | общая земля обязательна |
| `3V3/Vref` | 3V3 платы | только опорный уровень для ST-LINK |
| `NRST` | NRST | желательно подключить для надежной перезагрузки |

PA13/PA14 в проекте не заняты пользовательской периферией.

## 6. DMA и таймеры

| Узел | Настройка |
|---|---|
| TIM1 | PWM CH1 + complementary CH1N, выходы PE9/PE8 |
| TIM8 | PWM CH1 + complementary CH1N, выходы PC6/PA7 |
| TIM1 DMA | `TIM1_UP`, DMA2 Stream5 Channel6, обновляет PSC/ARR/CCR1 |
| TFT DMA | `SPI2_TX`, DMA1 Stream4 Channel0, передает framebuffer |
| Частота ядра | 168 MHz от HSI + PLL |
| APB2 timer clock | 168 MHz для TIM1/TIM8 |

## 7. Что включено в CubeMX `.ioc`

Файл `STM32f407vg_signal_generator.ioc` теперь содержит:

| CubeMX узел | Включено |
|---|---|
| `RCC` | HSI + PLL, SYSCLK 168 MHz, APB1 /4, APB2 /2 |
| `SYS` | SysTick |
| `GPIO` | PA0/PA1/PA2 input pull-up, PB10/PB11/PB12/PB14 output |
| `SPI2` | Master, transmit-only simplex, PB13/PB15 |
| `TIM1` | PWM Generation CH1 + CH1N |
| `TIM8` | PWM Generation CH1 + CH1N |
| `DMA` | SPI2_TX and TIM1_UP |
| `NVIC` | DMA1 Stream4, DMA2 Stream5 |

## 8. Упрощенная текстовая схема

```text
STM32F407VGT6

Power:
  3V3  ---------------- TFT VCC, Encoder VCC
  GND  ---------------- TFT GND, Encoder GND, Oscilloscope GND, ST-LINK GND

Signal outputs:
  PE9   TIM1_CH1   --[100..330R]-- OUT_A      -- Osc CH1
  PE8   TIM1_CH1N  --[100..330R]-- OUT_A_INV  -- Osc CH2
  PC6   TIM8_CH1   --[100..330R]-- OUT_B
  PA7   TIM8_CH1N  --[100..330R]-- OUT_B_INV

TFT ST7735:
  PB13  SPI2_SCK   ---------------- TFT SCK/SCL
  PB15  SPI2_MOSI  ---------------- TFT SDA/MOSI
  PB12  GPIO       ---------------- TFT CS
  PB14  GPIO       ---------------- TFT DC/A0
  PB10  GPIO       ---------------- TFT RST/RES
  PB11  GPIO       ---------------- TFT BL/LED

Encoder:
  PA0   GPIO PU    ---------------- Encoder S1/CLK
  PA1   GPIO PU    ---------------- Encoder S2/DT
  PA2   GPIO PU    ---------------- Encoder KEY/SW

Debug:
  PA13  SWDIO      ---------------- ST-LINK SWDIO
  PA14  SWCLK      ---------------- ST-LINK SWCLK
  NRST             ---------------- ST-LINK NRST
```
