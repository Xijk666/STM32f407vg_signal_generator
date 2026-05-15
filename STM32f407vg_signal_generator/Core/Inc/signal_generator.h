#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SIGNAL_GENERATOR_MIN_FREQ_HZ      1UL
#define SIGNAL_GENERATOR_MAX_FREQ_HZ      500000UL
#define SIGNAL_GENERATOR_DEFAULT_FREQ_HZ  1000UL
#define SIGNAL_GENERATOR_DEFAULT_WIDTH_NS 500000UL

typedef struct {
    uint32_t frequency_hz;
    uint32_t pulse_width_ns;
    uint16_t phase_tenths_deg;
} SignalGenerator_Config;

typedef struct {
    uint32_t timer_clock_hz;
    uint32_t counter_clock_hz;
    uint32_t prescaler;
    uint32_t period_ticks;
    uint32_t pulse_ticks;
    uint32_t phase_ticks;
} SignalGenerator_Runtime;

HAL_StatusTypeDef SignalGenerator_Init(TIM_HandleTypeDef *htim_a, TIM_HandleTypeDef *htim_b);
HAL_StatusTypeDef SignalGenerator_Start(void);
HAL_StatusTypeDef SignalGenerator_Apply(const SignalGenerator_Config *config);
const SignalGenerator_Config *SignalGenerator_GetConfig(void);
SignalGenerator_Runtime SignalGenerator_GetRuntime(void);
uint32_t SignalGenerator_GetPeriodNs(void);
uint32_t SignalGenerator_ClampWidthNs(uint32_t frequency_hz, uint32_t pulse_width_ns);

#ifdef __cplusplus
}
#endif

#endif
