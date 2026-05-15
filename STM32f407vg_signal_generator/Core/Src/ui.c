#include "ui.h"
#include "signal_generator.h"
#include "tft_st7735.h"
#include <stdio.h>
#include <string.h>

typedef enum {
    UI_FIELD_FREQ = 0,
    UI_FIELD_WIDTH,
    UI_FIELD_PHASE,
    UI_FIELD_STEP,
    UI_FIELD_COUNT
} UI_Field;

typedef enum {
    UI_FREQ_HZ = 0,
    UI_FREQ_PERIOD
} UI_FreqMode;

typedef enum {
    UI_WIDTH_NS = 0,
    UI_WIDTH_US,
    UI_WIDTH_MS,
    UI_WIDTH_S,
    UI_WIDTH_COUNT
} UI_TimeMode;

typedef enum {
    UI_PHASE_DEG = 0,
    UI_PHASE_TIME
} UI_PhaseMode;

typedef enum {
    UI_STEP_1HZ = 0,
    UI_STEP_5HZ,
    UI_STEP_10HZ,
    UI_STEP_50HZ,
    UI_STEP_100HZ,
    UI_STEP_250HZ,
    UI_STEP_500HZ,
    UI_STEP_1KHZ,
    UI_STEP_10KHZ,
    UI_STEP_COUNT
} UI_StepMode;

static Encoder_HandleTypeDef *ui_encoder;
static UI_Field selected_field;
static UI_FreqMode freq_mode;
static UI_TimeMode width_mode;
static UI_PhaseMode phase_mode;
static UI_StepMode step_mode;
static uint8_t dirty;

static const uint32_t freq_steps_hz[UI_STEP_COUNT] = {
    1U,
    5U,
    10U,
    50U,
    100U,
    250U,
    500U,
    1000U,
    10000U
};

static const uint32_t width_step_multipliers[UI_STEP_COUNT] = {
    1U,
    5U,
    10U,
    50U,
    100U,
    250U,
    500U,
    1000U,
    10000U
};

static const uint16_t phase_steps_tenths[UI_STEP_COUNT] = {
    1U,
    5U,
    10U,
    50U,
    100U,
    250U,
    500U,
    1000U,
    1800U
};

static const char *step_names[UI_STEP_COUNT] = {
    "1Hz",
    "5Hz",
    "10Hz",
    "50Hz",
    "100Hz",
    "250Hz",
    "500Hz",
    "1kHz",
    "10kHz"
};

static uint32_t clamp_frequency_i64(int64_t value)
{
    if (value < (int64_t)SIGNAL_GENERATOR_MIN_FREQ_HZ) {
        return SIGNAL_GENERATOR_MIN_FREQ_HZ;
    }
    if (value > (int64_t)SIGNAL_GENERATOR_MAX_FREQ_HZ) {
        return SIGNAL_GENERATOR_MAX_FREQ_HZ;
    }
    return (uint32_t)value;
}

static uint32_t frequency_step(void)
{
    return freq_steps_hz[step_mode];
}

static uint32_t width_step_ns(uint32_t period_ns)
{
    uint32_t step;

    if (period_ns <= 5000U) {
        step = 10U;
    } else if (period_ns <= 100000U) {
        step = 100U;
    } else if (period_ns <= 10000000U) {
        step = 1000U;
    } else {
        step = 100000U;
    }

    return step * width_step_multipliers[step_mode];
}

static void format_fixed(char *out, size_t out_len, uint32_t whole, uint32_t frac, uint32_t digits, const char *suffix)
{
    if (digits == 0U) {
        (void)snprintf(out, out_len, "%lu %s", (unsigned long)whole, suffix);
    } else if (digits == 1U) {
        (void)snprintf(out, out_len, "%lu.%01lu %s", (unsigned long)whole, (unsigned long)frac, suffix);
    } else if (digits == 2U) {
        (void)snprintf(out, out_len, "%lu.%02lu %s", (unsigned long)whole, (unsigned long)frac, suffix);
    } else {
        (void)snprintf(out, out_len, "%lu.%03lu %s", (unsigned long)whole, (unsigned long)frac, suffix);
    }
}

static void format_time_ns(char *out, size_t out_len, uint32_t ns, UI_TimeMode mode)
{
    switch (mode) {
    case UI_WIDTH_NS:
        format_fixed(out, out_len, ns, 0U, 0U, "ns");
        break;
    case UI_WIDTH_US:
        format_fixed(out, out_len, ns / 1000U, (ns % 1000U), 3U, "us");
        break;
    case UI_WIDTH_MS:
        format_fixed(out, out_len, ns / 1000000U, (ns % 1000000U) / 1000U, 3U, "ms");
        break;
    case UI_WIDTH_S:
    default:
        format_fixed(out, out_len, ns / 1000000000U, (ns % 1000000000U) / 1000000U, 3U, "s");
        break;
    }
}

static void format_frequency(char *out, size_t out_len, const SignalGenerator_Config *config)
{
    if (freq_mode == UI_FREQ_PERIOD) {
        format_time_ns(out, out_len, SignalGenerator_GetPeriodNs(), UI_WIDTH_US);
        return;
    }

    if (config->frequency_hz < 1000U) {
        (void)snprintf(out, out_len, "%lu Hz", (unsigned long)config->frequency_hz);
    } else {
        format_fixed(out,
                     out_len,
                     config->frequency_hz / 1000U,
                     config->frequency_hz % 1000U,
                     3U,
                     "kHz");
    }
}

static void apply_delta(int32_t delta)
{
    if (delta == 0) {
        return;
    }

    SignalGenerator_Config config = *SignalGenerator_GetConfig();

    if (selected_field == UI_FIELD_FREQ) {
        const uint32_t step = frequency_step();
        config.frequency_hz = clamp_frequency_i64((int64_t)config.frequency_hz + ((int64_t)delta * step));
        config.pulse_width_ns = SignalGenerator_ClampWidthNs(config.frequency_hz, config.pulse_width_ns);
    } else if (selected_field == UI_FIELD_WIDTH) {
        const uint32_t period_ns = (uint32_t)(1000000000ULL / config.frequency_hz);
        const uint32_t step = width_step_ns(period_ns);
        int64_t next_width = (int64_t)config.pulse_width_ns + ((int64_t)delta * step);
        if (next_width < 0) {
            next_width = 0;
        }
        config.pulse_width_ns = SignalGenerator_ClampWidthNs(config.frequency_hz, (uint32_t)next_width);
    } else if (selected_field == UI_FIELD_PHASE) {
        int32_t next_phase = (int32_t)config.phase_tenths_deg +
                             (delta * (int32_t)phase_steps_tenths[step_mode]);
        while (next_phase < 0) {
            next_phase += 3600;
        }
        config.phase_tenths_deg = (uint16_t)(next_phase % 3600);
    } else {
        int32_t next_step = (int32_t)step_mode + delta;
        while (next_step < 0) {
            next_step += (int32_t)UI_STEP_COUNT;
        }
        step_mode = (UI_StepMode)((uint32_t)next_step % (uint32_t)UI_STEP_COUNT);
        dirty = 1U;
        return;
    }

    if (SignalGenerator_Apply(&config) == HAL_OK) {
        dirty = 1U;
    }
}

void UI_Init(Encoder_HandleTypeDef *encoder)
{
    ui_encoder = encoder;
    selected_field = UI_FIELD_FREQ;
    freq_mode = UI_FREQ_HZ;
    width_mode = UI_WIDTH_US;
    phase_mode = UI_PHASE_DEG;
    step_mode = UI_STEP_1KHZ;
    dirty = 1U;
}

void UI_Task(uint32_t now_ms)
{
    (void)now_ms;

    apply_delta(Encoder_GetDelta(ui_encoder));

    if (Encoder_GetClick(ui_encoder)) {
        selected_field = (UI_Field)(((uint32_t)selected_field + 1U) % UI_FIELD_COUNT);
        dirty = 1U;
    }

    if (Encoder_GetLongPress(ui_encoder)) {
        if (selected_field == UI_FIELD_FREQ) {
            freq_mode = (freq_mode == UI_FREQ_HZ) ? UI_FREQ_PERIOD : UI_FREQ_HZ;
        } else if (selected_field == UI_FIELD_WIDTH) {
            width_mode = (UI_TimeMode)(((uint32_t)width_mode + 1U) % UI_WIDTH_COUNT);
        } else if (selected_field == UI_FIELD_PHASE) {
            phase_mode = (phase_mode == UI_PHASE_DEG) ? UI_PHASE_TIME : UI_PHASE_DEG;
        } else {
            step_mode = (UI_StepMode)(((uint32_t)step_mode + 1U) % UI_STEP_COUNT);
        }
        dirty = 1U;
    }
}

static void draw_row(uint16_t y, UI_Field field, const char *label, const char *value)
{
    char line[24];
    const uint8_t selected = (selected_field == field);
    const uint16_t bg = selected ? TFT_DARK : TFT_BLACK;
    const uint16_t fg = selected ? TFT_YELLOW : TFT_WHITE;

    TFT_FillRect(0U, y, TFT_WIDTH, 14U, bg);
    (void)snprintf(line, sizeof(line), "%c%-5s %s", selected ? '>' : ' ', label, value);
    TFT_DrawString(0U, y + 2U, line, fg, bg);
}

void UI_Render(void)
{
    char freq[20];
    char width[20];
    char phase[20];
    char step[20];
    char runtime[24];
    const SignalGenerator_Config *config = SignalGenerator_GetConfig();
    const SignalGenerator_Runtime gen = SignalGenerator_GetRuntime();

    if (!dirty || TFT_IsBusy()) {
        return;
    }

    format_frequency(freq, sizeof(freq), config);
    format_time_ns(width, sizeof(width), config->pulse_width_ns, width_mode);
    (void)snprintf(step, sizeof(step), "%s", step_names[step_mode]);

    if (phase_mode == UI_PHASE_DEG) {
        (void)snprintf(phase,
                       sizeof(phase),
                       "%u.%u deg",
                       (unsigned int)(config->phase_tenths_deg / 10U),
                       (unsigned int)(config->phase_tenths_deg % 10U));
    } else {
        const uint32_t delay_ns = (uint32_t)(((uint64_t)SignalGenerator_GetPeriodNs() *
                                              config->phase_tenths_deg) / 3600ULL);
        format_time_ns(phase, sizeof(phase), delay_ns, UI_WIDTH_US);
    }

    TFT_Clear(TFT_BLACK);
    TFT_FillRect(0U, 0U, TFT_WIDTH, 17U, TFT_BLUE);
    TFT_DrawString(3U, 4U, "STM32 SIGNAL GEN", TFT_WHITE, TFT_BLUE);

    draw_row(24U, UI_FIELD_FREQ, (freq_mode == UI_FREQ_HZ) ? "FREQ" : "PER", freq);
    draw_row(44U, UI_FIELD_WIDTH, "WIDTH", width);
    draw_row(64U, UI_FIELD_PHASE, "PHASE", phase);
    draw_row(84U, UI_FIELD_STEP, "STEP", step);

    (void)snprintf(runtime,
                   sizeof(runtime),
                   "PSC %lu ARR %lu",
                   (unsigned long)gen.prescaler,
                   (unsigned long)(gen.period_ticks - 1U));
    TFT_DrawString(0U, 104U, runtime, TFT_CYAN, TFT_BLACK);
    TFT_DrawString(0U, 120U, "A PE9 / PE8N", TFT_GREEN, TFT_BLACK);
    TFT_DrawString(0U, 136U, "B PC6 / PA7N", TFT_GREEN, TFT_BLACK);
    TFT_DrawString(0U, 152U, "TIM+DMA", TFT_ORANGE, TFT_BLACK);
    TFT_FillRect(82U, 150U, 44U, 10U, TFT_DARK);
    TFT_DrawString(84U, 152U, "By Xijk", TFT_YELLOW, TFT_DARK);

    if (TFT_Update() == HAL_OK) {
        dirty = 0U;
    }
}
