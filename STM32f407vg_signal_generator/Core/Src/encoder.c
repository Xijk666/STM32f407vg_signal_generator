#include "encoder.h"

#define ENCODER_AB_DEBOUNCE_MS      2U
#define ENCODER_KEY_DEBOUNCE_MS     25U
#define ENCODER_LONG_PRESS_MS       700U

static uint8_t encoder_read_ab(Encoder_HandleTypeDef *enc)
{
    const uint8_t s1 = (HAL_GPIO_ReadPin(enc->s1_port, enc->s1_pin) == GPIO_PIN_SET) ? 1U : 0U;
    const uint8_t s2 = (HAL_GPIO_ReadPin(enc->s2_port, enc->s2_pin) == GPIO_PIN_SET) ? 1U : 0U;
    return (uint8_t)((s1 << 1U) | s2);
}

static uint8_t encoder_read_key(Encoder_HandleTypeDef *enc)
{
    return (HAL_GPIO_ReadPin(enc->key_port, enc->key_pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

static int8_t encoder_transition(uint8_t old_state, uint8_t new_state)
{
    const uint8_t transition = (uint8_t)((old_state << 2U) | new_state);

    switch (transition) {
    case 0x1:
    case 0x7:
    case 0xE:
    case 0x8:
        return 1;
    case 0x2:
    case 0xB:
    case 0xD:
    case 0x4:
        return -1;
    default:
        return 0;
    }
}

void Encoder_Init(Encoder_HandleTypeDef *enc,
                  GPIO_TypeDef *s1_port, uint16_t s1_pin,
                  GPIO_TypeDef *s2_port, uint16_t s2_pin,
                  GPIO_TypeDef *key_port, uint16_t key_pin)
{
    enc->s1_port = s1_port;
    enc->s1_pin = s1_pin;
    enc->s2_port = s2_port;
    enc->s2_pin = s2_pin;
    enc->key_port = key_port;
    enc->key_pin = key_pin;

    enc->last_ab_sample = encoder_read_ab(enc);
    enc->stable_ab = enc->last_ab_sample;
    enc->ab_changed_ms = HAL_GetTick();
    enc->transition_accumulator = 0;
    enc->delta = 0;

    enc->last_key_sample = encoder_read_key(enc);
    enc->stable_key = enc->last_key_sample;
    enc->key_changed_ms = HAL_GetTick();
    enc->key_pressed_ms = 0U;
    enc->click_event = 0U;
    enc->long_press_event = 0U;
}

void Encoder_Poll(Encoder_HandleTypeDef *enc, uint32_t now_ms)
{
    const uint8_t ab_sample = encoder_read_ab(enc);

    if (ab_sample != enc->last_ab_sample) {
        enc->last_ab_sample = ab_sample;
        enc->ab_changed_ms = now_ms;
    } else if ((now_ms - enc->ab_changed_ms) >= ENCODER_AB_DEBOUNCE_MS &&
               ab_sample != enc->stable_ab) {
        const int8_t step = encoder_transition(enc->stable_ab, ab_sample);
        enc->stable_ab = ab_sample;
        enc->transition_accumulator = (int8_t)(enc->transition_accumulator + step);

        if (enc->transition_accumulator >= 4) {
            enc->delta++;
            enc->transition_accumulator = 0;
        } else if (enc->transition_accumulator <= -4) {
            enc->delta--;
            enc->transition_accumulator = 0;
        }
    }

    const uint8_t key_sample = encoder_read_key(enc);
    if (key_sample != enc->last_key_sample) {
        enc->last_key_sample = key_sample;
        enc->key_changed_ms = now_ms;
    } else if ((now_ms - enc->key_changed_ms) >= ENCODER_KEY_DEBOUNCE_MS &&
               key_sample != enc->stable_key) {
        enc->stable_key = key_sample;
        if (enc->stable_key) {
            enc->key_pressed_ms = now_ms;
        } else {
            const uint32_t held_ms = now_ms - enc->key_pressed_ms;
            if (held_ms >= ENCODER_LONG_PRESS_MS) {
                enc->long_press_event = 1U;
            } else {
                enc->click_event = 1U;
            }
        }
    }
}

int32_t Encoder_GetDelta(Encoder_HandleTypeDef *enc)
{
    const int32_t delta = enc->delta;
    enc->delta = 0;
    return delta;
}

uint8_t Encoder_GetClick(Encoder_HandleTypeDef *enc)
{
    const uint8_t event = enc->click_event;
    enc->click_event = 0U;
    return event;
}

uint8_t Encoder_GetLongPress(Encoder_HandleTypeDef *enc)
{
    const uint8_t event = enc->long_press_event;
    enc->long_press_event = 0U;
    return event;
}
