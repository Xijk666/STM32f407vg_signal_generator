#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

#define TFT_CS_Pin              GPIO_PIN_12
#define TFT_CS_GPIO_Port        GPIOB
#define TFT_DC_Pin              GPIO_PIN_14
#define TFT_DC_GPIO_Port        GPIOB
#define TFT_RST_Pin             GPIO_PIN_10
#define TFT_RST_GPIO_Port       GPIOB
#define TFT_BL_Pin              GPIO_PIN_11
#define TFT_BL_GPIO_Port        GPIOB

#define ENCODER_S1_Pin          GPIO_PIN_0
#define ENCODER_S1_GPIO_Port    GPIOA
#define ENCODER_S2_Pin          GPIO_PIN_1
#define ENCODER_S2_GPIO_Port    GPIOA
#define ENCODER_KEY_Pin         GPIO_PIN_2
#define ENCODER_KEY_GPIO_Port   GPIOA

#define SIG_A_Pin               GPIO_PIN_9
#define SIG_A_GPIO_Port         GPIOE
#define SIG_A_INV_Pin           GPIO_PIN_8
#define SIG_A_INV_GPIO_Port     GPIOE
#define SIG_B_Pin               GPIO_PIN_6
#define SIG_B_GPIO_Port         GPIOC
#define SIG_B_INV_Pin           GPIO_PIN_7
#define SIG_B_INV_GPIO_Port     GPIOA

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif
