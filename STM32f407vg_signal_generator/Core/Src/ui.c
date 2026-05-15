#include "ui.h"
#include "signal_generator.h"
#include "tft_st7735.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef enum {
	UI_FIELD_FREQ  = 0,
	UI_FIELD_WIDTH,
	UI_FIELD_PHASE,
	UI_FIELD_STEP,
	UI_FIELD_COUNT
} UI_Field;

typedef enum {
	UI_FREQ_HZ     = 0,
	UI_FREQ_PERIOD
} UI_FreqMode;

typedef enum {
	UI_WIDTH_NS    = 0,
	UI_WIDTH_US,
	UI_WIDTH_MS,
	UI_WIDTH_S,
	UI_WIDTH_COUNT
} UI_TimeMode;

typedef enum {
	UI_PHASE_DEG  = 0,
	UI_PHASE_TIME
} UI_PhaseMode;

typedef enum {
	UI_STEP_1HZ   = 0,
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

/*
 * Подпись справа.
 *
 * Теперь буквы НЕ столбиком.
 * Теперь вся надпись повернута на 90 градусов
 * и лежит на правой стенке дисплея.
 */
#define UI_SIGNATURE_TEXT          "By Xijk666"

#define UI_ROT_FONT_W              5U
#define UI_ROT_FONT_H              7U
#define UI_ROT_CHAR_STEP           6U

/*
 * Ширина закрашенной зоны справа.
 * 7 пикселей = высота повернутого символа.
 */
#define UI_SIGNATURE_BOX_W         UI_ROT_FONT_H

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
	}
	else if (period_ns <= 100000U) {
		step = 100U;
	}
	else if (period_ns <= 10000000U) {
		step = 1000U;
	}
	else {
		step = 100000U;
	}

	return step * width_step_multipliers[step_mode];
}

static void format_fixed(char *out,
	size_t out_len,
	uint32_t whole,
	uint32_t frac,
	uint32_t digits,
	const char *suffix)
{
	if (digits == 0U) {
		(void)snprintf(out,
			out_len,
			"%lu %s",
			(unsigned long)whole,
			suffix);
	}
	else if (digits == 1U) {
		(void)snprintf(out,
			out_len,
			"%lu.%01lu %s",
			(unsigned long)whole,
			(unsigned long)frac,
			suffix);
	}
	else if (digits == 2U) {
		(void)snprintf(out,
			out_len,
			"%lu.%02lu %s",
			(unsigned long)whole,
			(unsigned long)frac,
			suffix);
	}
	else {
		(void)snprintf(out,
			out_len,
			"%lu.%03lu %s",
			(unsigned long)whole,
			(unsigned long)frac,
			suffix);
	}
}

static void format_time_ns(char *out, size_t out_len, uint32_t ns, UI_TimeMode mode)
{
	switch (mode) {
	case UI_WIDTH_NS:
		format_fixed(out, out_len, ns, 0U, 0U, "ns");
		break;

	case UI_WIDTH_US:
		format_fixed(out,
			out_len,
			ns / 1000U,
			ns % 1000U,
			3U,
			"us");
		break;

	case UI_WIDTH_MS:
		format_fixed(out,
			out_len,
			ns / 1000000U,
			(ns % 1000000U) / 1000U,
			3U,
			"ms");
		break;

	case UI_WIDTH_S:
	default:
		format_fixed(out,
			out_len,
			ns / 1000000000U,
			(ns % 1000000000U) / 1000000U,
			3U,
			"s");
		break;
	}
}

static void format_frequency(char *out,
	size_t out_len,
	const SignalGenerator_Config *config)
{
	if (freq_mode == UI_FREQ_PERIOD) {
		format_time_ns(out, out_len, SignalGenerator_GetPeriodNs(), UI_WIDTH_US);
		return;
	}

	if (config->frequency_hz < 1000U) {
		(void)snprintf(out,
			out_len,
			"%lu Hz",
			(unsigned long)config->frequency_hz);
	}
	else {
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

		config.frequency_hz = clamp_frequency_i64(
		    (int64_t)config.frequency_hz + ((int64_t)delta * step)
		);

		config.pulse_width_ns = SignalGenerator_ClampWidthNs(
		    config.frequency_hz,
			config.pulse_width_ns);

	}
	else if (selected_field == UI_FIELD_WIDTH) {
		const uint32_t period_ns = (uint32_t)(1000000000ULL / config.frequency_hz);
		const uint32_t step = width_step_ns(period_ns);

		int64_t next_width = (int64_t)config.pulse_width_ns +
		                     ((int64_t)delta * step);

		if (next_width < 0) {
			next_width = 0;
		}

		config.pulse_width_ns = SignalGenerator_ClampWidthNs(
		    config.frequency_hz,
			(uint32_t)next_width);

	}
	else if (selected_field == UI_FIELD_PHASE) {
		int32_t next_phase = (int32_t)config.phase_tenths_deg +
		                     (delta * (int32_t)phase_steps_tenths[step_mode]);

		while (next_phase < 0) {
			next_phase += 3600;
		}

		config.phase_tenths_deg = (uint16_t)(next_phase % 3600);

	}
	else {
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
		}
		else if (selected_field == UI_FIELD_WIDTH) {
			width_mode = (UI_TimeMode)(((uint32_t)width_mode + 1U) % UI_WIDTH_COUNT);
		}
		else if (selected_field == UI_FIELD_PHASE) {
			phase_mode = (phase_mode == UI_PHASE_DEG) ? UI_PHASE_TIME : UI_PHASE_DEG;
		}
		else {
			step_mode = (UI_StepMode)(((uint32_t)step_mode + 1U) % UI_STEP_COUNT);
		}

		dirty = 1U;
	}
}

static void draw_row(uint16_t y,
	UI_Field field,
	const char *label,
	const char *value)
{
	char line[24];

	const uint8_t selected = (selected_field == field);
	const uint16_t bg = selected ? TFT_DARK : TFT_BLACK;
	const uint16_t fg = selected ? TFT_YELLOW : TFT_WHITE;

	TFT_FillRect(0U, y, TFT_WIDTH, 14U, bg);

	(void)snprintf(line,
		sizeof(line),
		"%c%-5s %s",
		selected ? '>' : ' ',
		label,
		value);

	TFT_DrawString(0U, y + 2U, line, fg, bg);
}

/*
 * Мини-шрифт 5x7 только для подписи "By Xijk666".
 * Каждый байт — одна строка символа.
 * Используются только младшие 5 бит.
 */
static const uint8_t *signature_get_glyph(char c)
{
	static const uint8_t glyph_space[7] = {
		0b00000,
		0b00000,
		0b00000,
		0b00000,
		0b00000,
		0b00000,
		0b00000
	};

	static const uint8_t glyph_B[7] = {
		0b11110,
		0b10001,
		0b10001,
		0b11110,
		0b10001,
		0b10001,
		0b11110
	};

	static const uint8_t glyph_y[7] = {
		0b00000,
		0b10001,
		0b10001,
		0b01111,
		0b00001,
		0b10001,
		0b01110
	};

	static const uint8_t glyph_X[7] = {
		0b10001,
		0b10001,
		0b01010,
		0b00100,
		0b01010,
		0b10001,
		0b10001
	};

	static const uint8_t glyph_i[7] = {
		0b00100,
		0b00000,
		0b01100,
		0b00100,
		0b00100,
		0b00100,
		0b01110
	};

	static const uint8_t glyph_j[7] = {
		0b00010,
		0b00000,
		0b00110,
		0b00010,
		0b00010,
		0b10010,
		0b01100
	};

	static const uint8_t glyph_k[7] = {
		0b10000,
		0b10000,
		0b10010,
		0b10100,
		0b11000,
		0b10100,
		0b10010
	};

	static const uint8_t glyph_6[7] = {
		0b01110,
		0b10000,
		0b10000,
		0b11110,
		0b10001,
		0b10001,
		0b01110
	};

	switch (c) {
	case 'B':
		return glyph_B;

	case 'y':
		return glyph_y;

	case 'X':
		return glyph_X;

	case 'i':
		return glyph_i;

	case 'j':
		return glyph_j;

	case 'k':
		return glyph_k;

	case '6':
		return glyph_6;

	case ' ':
	default:
		return glyph_space;
	}
}

/*
 * Рисует подпись справа, повернутую на 90 градусов.
 *
 * Важно:
 * - текст начинается от правого нижнего угла;
 * - буквы лежат на правой стенке;
 * - фон закрашивается только под подписью;
 * - используется TFT_FillRect(x, y, 1, 1, color),
 *   поэтому отдельный TFT_DrawPixel не нужен.
 */
static void draw_signature_right_rotated(void)
{
	const char *text = UI_SIGNATURE_TEXT;
	size_t len = strlen(text);

	uint16_t text_len_px;
	uint16_t box_x;
	uint16_t box_y;
	uint16_t box_w;
	uint16_t box_h;

	size_t char_index;
	uint8_t row;
	uint8_t col;

	if (len == 0U) {
		return;
	}

	/*
	 * Длина повернутого текста по вертикали.
	 * Каждый символ занимает 6 пикселей:
	 * 5 пикселей буква + 1 пиксель промежуток.
	 */
	text_len_px = (uint16_t)((len * UI_ROT_CHAR_STEP) - 1U);

	/*
	 * Если текст слишком длинный — обрезаем под высоту экрана.
	 */
	while (text_len_px > TFT_HEIGHT && len > 0U) {
		len--;
		text_len_px = (uint16_t)((len * UI_ROT_CHAR_STEP) - 1U);
	}

	if (len == 0U) {
		return;
	}

	/*
	 * Повернутый символ 5x7 занимает:
	 * ширина по X = 7 пикселей,
	 * высота по Y = длина всей строки.
	 */
	box_w = UI_SIGNATURE_BOX_W;
	box_h = text_len_px;

	/*
	 * Правая стенка:
	 * фон начинается с левой стороны правой стенки
	 * и заканчивается прямо на правом краю дисплея.
	 */
	box_x = TFT_WIDTH - box_w;

	/*
	 * Старт от правого нижнего угла:
	 * подпись прижата вниз.
	 */
	box_y = TFT_HEIGHT - box_h;

	/*
	 * Закрашиваем ТОЛЬКО область под подписью.
	 */
	TFT_FillRect(box_x, box_y, box_w, box_h, TFT_DARK);

	/*
	 * Поворот 90 градусов против часовой стрелки:
	 *
	 * Обычный текст:
	 * B y   X i j k 6 6 6
	 *
	 * После поворота:
	 * текст лежит на правой стенке,
	 * первая буква начинается снизу.
	 */
	for (char_index = 0U; char_index < len; char_index++) {
		const uint8_t *glyph = signature_get_glyph(text[char_index]);

		for (row = 0U; row < UI_ROT_FONT_H; row++) {
			for (col = 0U; col < UI_ROT_FONT_W; col++) {
				uint8_t pixel_on;
				uint16_t normal_x;
				uint16_t rotated_x;
				uint16_t rotated_y;
				uint16_t draw_x;
				uint16_t draw_y;

				pixel_on = (glyph[row] & (uint8_t)(1U << (UI_ROT_FONT_W - 1U - col))) ? 1U : 0U;

				if (!pixel_on) {
					continue;
				}

				/*
				 * Позиция пикселя в обычной горизонтальной строке.
				 */
				normal_x = (uint16_t)((char_index * UI_ROT_CHAR_STEP) + col);

				/*
				 * Поворот 90 градусов против часовой:
				 * X становится строкой символа,
				 * Y идет снизу вверх.
				 */
				rotated_x = row;
				rotated_y = (uint16_t)((text_len_px - 1U) - normal_x);

				draw_x = box_x + rotated_x;
				draw_y = box_y + rotated_y;

				if (draw_x < TFT_WIDTH && draw_y < TFT_HEIGHT) {
					TFT_FillRect(draw_x, draw_y, 1U, 1U, TFT_YELLOW);
				}
			}
		}
	}
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
	}
	else {
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

	/*
	 * Повернутая подпись справа.
	 */
	draw_signature_right_rotated();

	if (TFT_Update() == HAL_OK) {
		dirty = 0U;
	}
}