#ifndef ENCODER_H
#define ENCODER_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    GPIO_TypeDef *s1_port;
    uint16_t s1_pin;
    GPIO_TypeDef *s2_port;
    uint16_t s2_pin;
    GPIO_TypeDef *key_port;
    uint16_t key_pin;

    uint8_t last_ab_sample;
    uint8_t stable_ab;
    uint32_t ab_changed_ms;
    int8_t transition_accumulator;
    volatile int32_t delta;

    uint8_t last_key_sample;
    uint8_t stable_key;
    uint32_t key_changed_ms;
    uint32_t key_pressed_ms;
    volatile uint8_t click_event;
    volatile uint8_t long_press_event;
} Encoder_HandleTypeDef;

void Encoder_Init(Encoder_HandleTypeDef *enc,
                  GPIO_TypeDef *s1_port, uint16_t s1_pin,
                  GPIO_TypeDef *s2_port, uint16_t s2_pin,
                  GPIO_TypeDef *key_port, uint16_t key_pin);
void Encoder_Poll(Encoder_HandleTypeDef *enc, uint32_t now_ms);
int32_t Encoder_GetDelta(Encoder_HandleTypeDef *enc);
uint8_t Encoder_GetClick(Encoder_HandleTypeDef *enc);
uint8_t Encoder_GetLongPress(Encoder_HandleTypeDef *enc);

#ifdef __cplusplus
}
#endif

#endif
