#include "signal_generator.h"

#define TIMER_REGISTER_MAX            65535UL
#define TIMER_DMA_BURST_WORDS         4U

static TIM_HandleTypeDef *timer_a;
static TIM_HandleTypeDef *timer_b;
static SignalGenerator_Config active_config;
static SignalGenerator_Runtime active_runtime;
static uint32_t tim1_burst[TIMER_DMA_BURST_WORDS];
static uint8_t generator_running;

static uint32_t get_timer_clock_hz(TIM_HandleTypeDef *htim)
{
    uint32_t pclk_hz;
    uint32_t ppre_bits;

    if (htim->Instance == TIM1 || htim->Instance == TIM8 ||
        htim->Instance == TIM9 || htim->Instance == TIM10 ||
        htim->Instance == TIM11) {
        pclk_hz = HAL_RCC_GetPCLK2Freq();
        ppre_bits = (RCC->CFGR & RCC_CFGR_PPRE2);
    } else {
        pclk_hz = HAL_RCC_GetPCLK1Freq();
        ppre_bits = (RCC->CFGR & RCC_CFGR_PPRE1);
    }

    return (ppre_bits == 0U) ? pclk_hz : (pclk_hz * 2U);
}

static uint32_t clamp_u32(uint32_t value, uint32_t low, uint32_t high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

uint32_t SignalGenerator_ClampWidthNs(uint32_t frequency_hz, uint32_t pulse_width_ns)
{
    frequency_hz = clamp_u32(frequency_hz,
                             SIGNAL_GENERATOR_MIN_FREQ_HZ,
                             SIGNAL_GENERATOR_MAX_FREQ_HZ);

    const uint32_t period_ns = (uint32_t)(1000000000ULL / frequency_hz);
    const uint32_t min_width_ns = 20U;
    const uint32_t max_width_ns = (period_ns > min_width_ns) ? (period_ns - min_width_ns) : period_ns;

    return clamp_u32(pulse_width_ns, min_width_ns, max_width_ns);
}

static SignalGenerator_Runtime calculate_runtime(const SignalGenerator_Config *config)
{
    SignalGenerator_Runtime runtime = {0};
    const uint32_t frequency_hz = clamp_u32(config->frequency_hz,
                                            SIGNAL_GENERATOR_MIN_FREQ_HZ,
                                            SIGNAL_GENERATOR_MAX_FREQ_HZ);
    const uint32_t timer_clock_hz = get_timer_clock_hz(timer_a);

    uint32_t prescaler = (uint32_t)((timer_clock_hz + ((uint64_t)frequency_hz * 65536ULL) - 1ULL) /
                                    ((uint64_t)frequency_hz * 65536ULL));
    if (prescaler > 0U) {
        prescaler--;
    }

    uint32_t counter_clock_hz = timer_clock_hz / (prescaler + 1U);
    uint32_t period_ticks = (uint32_t)(((uint64_t)counter_clock_hz + (frequency_hz / 2U)) / frequency_hz);
    period_ticks = clamp_u32(period_ticks, 2U, TIMER_REGISTER_MAX + 1U);

    const uint32_t pulse_width_ns = SignalGenerator_ClampWidthNs(frequency_hz, config->pulse_width_ns);
    uint32_t pulse_ticks = (uint32_t)(((uint64_t)pulse_width_ns * counter_clock_hz + 999999999ULL) /
                                      1000000000ULL);
    pulse_ticks = clamp_u32(pulse_ticks, 1U, period_ticks - 1U);

    const uint32_t phase_tenths = config->phase_tenths_deg % 3600U;
    const uint32_t phase_ticks = (uint32_t)(((uint64_t)period_ticks * phase_tenths + 1800ULL) / 3600ULL);

    runtime.timer_clock_hz = timer_clock_hz;
    runtime.counter_clock_hz = counter_clock_hz;
    runtime.prescaler = prescaler;
    runtime.period_ticks = period_ticks;
    runtime.pulse_ticks = pulse_ticks;
    runtime.phase_ticks = (phase_ticks >= period_ticks) ? 0U : phase_ticks;
    return runtime;
}

static void write_timer_direct(TIM_HandleTypeDef *htim,
                               const SignalGenerator_Runtime *runtime)
{
    __HAL_TIM_SET_PRESCALER(htim, runtime->prescaler);
    __HAL_TIM_SET_AUTORELOAD(htim, runtime->period_ticks - 1U);
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, runtime->pulse_ticks);
}

static void apply_phase_to_timer_b(const SignalGenerator_Runtime *runtime)
{
    const uint32_t counter_start = (runtime->phase_ticks == 0U) ?
                                   0U :
                                   ((runtime->period_ticks - runtime->phase_ticks) % runtime->period_ticks);
    __HAL_TIM_SET_COUNTER(timer_b, counter_start);
}

HAL_StatusTypeDef SignalGenerator_Init(TIM_HandleTypeDef *htim_a, TIM_HandleTypeDef *htim_b)
{
    timer_a = htim_a;
    timer_b = htim_b;
    generator_running = 0U;

    active_config.frequency_hz = SIGNAL_GENERATOR_DEFAULT_FREQ_HZ;
    active_config.pulse_width_ns = SIGNAL_GENERATOR_DEFAULT_WIDTH_NS;
    active_config.phase_tenths_deg = 0U;

    active_runtime = calculate_runtime(&active_config);
    write_timer_direct(timer_a, &active_runtime);
    write_timer_direct(timer_b, &active_runtime);
    apply_phase_to_timer_b(&active_runtime);

    return HAL_OK;
}

HAL_StatusTypeDef SignalGenerator_Start(void)
{
    HAL_StatusTypeDef status;

    status = HAL_TIM_PWM_Start(timer_a, TIM_CHANNEL_1);
    if (status != HAL_OK) {
        return status;
    }
    status = HAL_TIMEx_PWMN_Start(timer_a, TIM_CHANNEL_1);
    if (status != HAL_OK) {
        return status;
    }
    status = HAL_TIM_PWM_Start(timer_b, TIM_CHANNEL_1);
    if (status != HAL_OK) {
        return status;
    }
    status = HAL_TIMEx_PWMN_Start(timer_b, TIM_CHANNEL_1);
    if (status != HAL_OK) {
        return status;
    }

    generator_running = 1U;
    return HAL_OK;
}

HAL_StatusTypeDef SignalGenerator_Apply(const SignalGenerator_Config *config)
{
    SignalGenerator_Config next_config = *config;
    HAL_StatusTypeDef status;

    next_config.frequency_hz = clamp_u32(next_config.frequency_hz,
                                         SIGNAL_GENERATOR_MIN_FREQ_HZ,
                                         SIGNAL_GENERATOR_MAX_FREQ_HZ);
    next_config.pulse_width_ns = SignalGenerator_ClampWidthNs(next_config.frequency_hz,
                                                              next_config.pulse_width_ns);
    next_config.phase_tenths_deg %= 3600U;

    SignalGenerator_Runtime next_runtime = calculate_runtime(&next_config);

    if (!generator_running) {
        write_timer_direct(timer_a, &next_runtime);
        write_timer_direct(timer_b, &next_runtime);
        HAL_TIM_GenerateEvent(timer_b, TIM_EVENTSOURCE_UPDATE);
        apply_phase_to_timer_b(&next_runtime);
        active_config = next_config;
        active_runtime = next_runtime;
        return HAL_OK;
    }

    tim1_burst[0] = next_runtime.prescaler;
    tim1_burst[1] = next_runtime.period_ticks - 1U;
    tim1_burst[2] = 0U;
    tim1_burst[3] = next_runtime.pulse_ticks;

    status = HAL_TIM_DMABurst_WriteStart(timer_a,
                                         TIM_DMABASE_PSC,
                                         TIM_DMA_UPDATE,
                                         tim1_burst,
                                         TIM_DMABURSTLENGTH_4TRANSFERS);
    if (status == HAL_BUSY) {
        write_timer_direct(timer_a, &next_runtime);
        HAL_TIM_GenerateEvent(timer_a, TIM_EVENTSOURCE_UPDATE);
        status = HAL_OK;
    }

    if (status == HAL_OK) {
        write_timer_direct(timer_b, &next_runtime);
        HAL_TIM_GenerateEvent(timer_b, TIM_EVENTSOURCE_UPDATE);
        apply_phase_to_timer_b(&next_runtime);
        active_config = next_config;
        active_runtime = next_runtime;
    }

    return status;
}

const SignalGenerator_Config *SignalGenerator_GetConfig(void)
{
    return &active_config;
}

SignalGenerator_Runtime SignalGenerator_GetRuntime(void)
{
    return active_runtime;
}

uint32_t SignalGenerator_GetPeriodNs(void)
{
    return (uint32_t)(1000000000ULL / active_config.frequency_hz);
}
