#ifndef TFT_ST7735_H
#define TFT_ST7735_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TFT_WIDTH   128U
#define TFT_HEIGHT  160U

/* Most 1.8" 128x160 ST7735 modules map the visible glass with a small RAM offset.
 * Tune these two values if the image is still shifted on your exact display.
 */
#define TFT_COL_OFFSET 2U
#define TFT_ROW_OFFSET 1U

#define TFT_BLACK   0x0000U
#define TFT_WHITE   0xFFFFU
#define TFT_RED     0xF800U
#define TFT_GREEN   0x07E0U
#define TFT_BLUE    0x001FU
#define TFT_CYAN    0x07FFU
#define TFT_YELLOW  0xFFE0U
#define TFT_ORANGE  0xFD20U
#define TFT_DARK    0x18E3U

void TFT_Init(SPI_HandleTypeDef *hspi);
void TFT_Clear(uint16_t color);
void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void TFT_DrawString(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg);
HAL_StatusTypeDef TFT_Update(void);
uint8_t TFT_IsBusy(void);

#ifdef __cplusplus
}
#endif

#endif
