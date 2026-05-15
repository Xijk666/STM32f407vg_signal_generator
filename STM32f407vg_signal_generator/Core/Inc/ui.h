#ifndef UI_H
#define UI_H

#include "encoder.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void UI_Init(Encoder_HandleTypeDef *encoder);
void UI_Task(uint32_t now_ms);
void UI_Render(void);

#ifdef __cplusplus
}
#endif

#endif
