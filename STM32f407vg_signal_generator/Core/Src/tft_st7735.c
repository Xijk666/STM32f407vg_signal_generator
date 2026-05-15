#include "tft_st7735.h"
#include "main.h"
#include <string.h>

#define ST7735_SWRESET 0x01U
#define ST7735_SLPOUT  0x11U
#define ST7735_COLMOD  0x3AU
#define ST7735_MADCTL  0x36U
#define ST7735_CASET   0x2AU
#define ST7735_RASET   0x2BU
#define ST7735_RAMWR   0x2CU
#define ST7735_DISPON  0x29U

#define MADCTL_MX      0x40U
#define MADCTL_MY      0x80U
#define MADCTL_RGB     0x00U

#define FONT_WIDTH     5U
#define FONT_HEIGHT    7U
#define FONT_SPACING   1U

static SPI_HandleTypeDef *tft_spi;
static volatile uint8_t tft_dma_busy;
static uint16_t framebuffer[TFT_WIDTH * TFT_HEIGHT];

static uint16_t bus_color(uint16_t color)
{
    return (uint16_t)((color << 8U) | (color >> 8U));
}

static void cs_low(void)
{
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET);
}

static void cs_high(void)
{
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);
}

static void dc_command(void)
{
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET);
}

static void dc_data(void)
{
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET);
}

static void write_command(uint8_t command)
{
    while (tft_dma_busy) {
    }

    dc_command();
    cs_low();
    (void)HAL_SPI_Transmit(tft_spi, &command, 1U, HAL_MAX_DELAY);
    cs_high();
}

static void write_data(const uint8_t *data, uint16_t length)
{
    while (tft_dma_busy) {
    }

    dc_data();
    cs_low();
    (void)HAL_SPI_Transmit(tft_spi, (uint8_t *)data, length, HAL_MAX_DELAY);
    cs_high();
}

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    x0 = (uint16_t)(x0 + TFT_COL_OFFSET);
    x1 = (uint16_t)(x1 + TFT_COL_OFFSET);
    y0 = (uint16_t)(y0 + TFT_ROW_OFFSET);
    y1 = (uint16_t)(y1 + TFT_ROW_OFFSET);

    write_command(ST7735_CASET);
    data[0] = (uint8_t)(x0 >> 8U);
    data[1] = (uint8_t)x0;
    data[2] = (uint8_t)(x1 >> 8U);
    data[3] = (uint8_t)x1;
    write_data(data, sizeof(data));

    write_command(ST7735_RASET);
    data[0] = (uint8_t)(y0 >> 8U);
    data[1] = (uint8_t)y0;
    data[2] = (uint8_t)(y1 >> 8U);
    data[3] = (uint8_t)y1;
    write_data(data, sizeof(data));

    write_command(ST7735_RAMWR);
}

#define GLYPH(a, b, c, d, e, f, g) \
    do { \
        rows[0] = (a); rows[1] = (b); rows[2] = (c); rows[3] = (d); \
        rows[4] = (e); rows[5] = (f); rows[6] = (g); \
    } while (0)

static void glyph_rows(char ch, uint8_t rows[FONT_HEIGHT])
{
    memset(rows, 0, FONT_HEIGHT);
    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 'a' + 'A');
    }

    switch (ch) {
    case '0': GLYPH(0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E); break;
    case '1': GLYPH(0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E); break;
    case '2': GLYPH(0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F); break;
    case '3': GLYPH(0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E); break;
    case '4': GLYPH(0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02); break;
    case '5': GLYPH(0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E); break;
    case '6': GLYPH(0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E); break;
    case '7': GLYPH(0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08); break;
    case '8': GLYPH(0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E); break;
    case '9': GLYPH(0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E); break;
    case 'A': GLYPH(0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11); break;
    case 'B': GLYPH(0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E); break;
    case 'C': GLYPH(0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E); break;
    case 'D': GLYPH(0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E); break;
    case 'E': GLYPH(0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F); break;
    case 'F': GLYPH(0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10); break;
    case 'G': GLYPH(0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F); break;
    case 'H': GLYPH(0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11); break;
    case 'I': GLYPH(0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E); break;
    case 'J': GLYPH(0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C); break;
    case 'K': GLYPH(0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11); break;
    case 'L': GLYPH(0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F); break;
    case 'M': GLYPH(0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11); break;
    case 'N': GLYPH(0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11); break;
    case 'O': GLYPH(0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E); break;
    case 'P': GLYPH(0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10); break;
    case 'Q': GLYPH(0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D); break;
    case 'R': GLYPH(0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11); break;
    case 'S': GLYPH(0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E); break;
    case 'T': GLYPH(0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04); break;
    case 'U': GLYPH(0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E); break;
    case 'V': GLYPH(0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04); break;
    case 'W': GLYPH(0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A); break;
    case 'X': GLYPH(0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11); break;
    case 'Y': GLYPH(0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04); break;
    case 'Z': GLYPH(0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F); break;
    case '>': GLYPH(0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10); break;
    case '/': GLYPH(0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10); break;
    case '.': GLYPH(0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C); break;
    case ':': GLYPH(0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00); break;
    case '+': GLYPH(0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00); break;
    case '-': GLYPH(0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00); break;
    case '_': GLYPH(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F); break;
    case ' ': break;
    default:  GLYPH(0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04); break;
    }
}

void TFT_Init(SPI_HandleTypeDef *hspi)
{
    tft_spi = hspi;
    tft_dma_busy = 0U;

    HAL_GPIO_WritePin(TFT_BL_GPIO_Port, TFT_BL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(20U);
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(120U);

    write_command(ST7735_SWRESET);
    HAL_Delay(150U);

    write_command(ST7735_SLPOUT);
    HAL_Delay(120U);

    {
        const uint8_t color_mode = 0x05U;
        write_command(ST7735_COLMOD);
        write_data(&color_mode, 1U);
    }

    {
        const uint8_t madctl = (uint8_t)(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
        write_command(ST7735_MADCTL);
        write_data(&madctl, 1U);
    }

    write_command(ST7735_DISPON);
    HAL_Delay(100U);

    TFT_Clear(TFT_BLACK);
    (void)TFT_Update();
}

void TFT_Clear(uint16_t color)
{
    const uint16_t packed = bus_color(color);
    for (uint32_t i = 0; i < (TFT_WIDTH * TFT_HEIGHT); i++) {
        framebuffer[i] = packed;
    }
}

void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
        return;
    }

    framebuffer[(uint32_t)y * TFT_WIDTH + x] = bus_color(color);
}

void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
        return;
    }

    if ((x + w) > TFT_WIDTH) {
        w = (uint16_t)(TFT_WIDTH - x);
    }
    if ((y + h) > TFT_HEIGHT) {
        h = (uint16_t)(TFT_HEIGHT - y);
    }

    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            TFT_DrawPixel((uint16_t)(x + col), (uint16_t)(y + row), color);
        }
    }
}

static void draw_char(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg)
{
    uint8_t rows[FONT_HEIGHT];
    glyph_rows(ch, rows);

    for (uint8_t row = 0; row < FONT_HEIGHT; row++) {
        for (uint8_t col = 0; col < FONT_WIDTH; col++) {
            const uint8_t mask = (uint8_t)(1U << (FONT_WIDTH - 1U - col));
            const uint16_t pixel_color = (rows[row] & mask) ? color : bg;
            TFT_DrawPixel((uint16_t)(x + col), (uint16_t)(y + row), pixel_color);
        }
    }
}

void TFT_DrawString(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg)
{
    uint16_t cursor_x = x;
    uint16_t cursor_y = y;

    while (*text != '\0') {
        if (*text == '\n') {
            cursor_x = x;
            cursor_y = (uint16_t)(cursor_y + FONT_HEIGHT + 2U);
            text++;
            continue;
        }

        if ((cursor_x + FONT_WIDTH) >= TFT_WIDTH) {
            cursor_x = x;
            cursor_y = (uint16_t)(cursor_y + FONT_HEIGHT + 2U);
        }

        if ((cursor_y + FONT_HEIGHT) >= TFT_HEIGHT) {
            return;
        }

        draw_char(cursor_x, cursor_y, *text, color, bg);
        cursor_x = (uint16_t)(cursor_x + FONT_WIDTH + FONT_SPACING);
        text++;
    }
}

HAL_StatusTypeDef TFT_Update(void)
{
    if (tft_dma_busy) {
        return HAL_BUSY;
    }

    set_window(0U, 0U, TFT_WIDTH - 1U, TFT_HEIGHT - 1U);

    tft_dma_busy = 1U;
    dc_data();
    cs_low();

    if (HAL_SPI_Transmit_DMA(tft_spi,
                             (uint8_t *)framebuffer,
                             (uint16_t)sizeof(framebuffer)) != HAL_OK) {
        tft_dma_busy = 0U;
        cs_high();
        return HAL_ERROR;
    }

    return HAL_OK;
}

uint8_t TFT_IsBusy(void)
{
    return tft_dma_busy;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == tft_spi) {
        tft_dma_busy = 0U;
        cs_high();
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == tft_spi) {
        tft_dma_busy = 0U;
        cs_high();
    }
}
