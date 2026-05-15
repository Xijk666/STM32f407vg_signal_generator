#include "main.h"

extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_tim1_up;

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hspi->Instance != SPI2) {
        return;
    }

    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hdma_spi2_tx.Instance = DMA1_Stream4;
    hdma_spi2_tx.Init.Channel = DMA_CHANNEL_0;
    hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi2_tx.Init.Mode = DMA_NORMAL;
    hdma_spi2_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_spi2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spi2_tx) != HAL_OK) {
        Error_Handler();
    }

    __HAL_LINKDMA(hspi, hdmatx, hdma_spi2_tx);
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance != SPI2) {
        return;
    }

    __HAL_RCC_SPI2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13 | GPIO_PIN_15);
    HAL_DMA_DeInit(hspi->hdmatx);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        __HAL_RCC_TIM1_CLK_ENABLE();
        __HAL_RCC_DMA2_CLK_ENABLE();

        hdma_tim1_up.Instance = DMA2_Stream5;
        hdma_tim1_up.Init.Channel = DMA_CHANNEL_6;
        hdma_tim1_up.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_tim1_up.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_tim1_up.Init.MemInc = DMA_MINC_ENABLE;
        hdma_tim1_up.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma_tim1_up.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma_tim1_up.Init.Mode = DMA_NORMAL;
        hdma_tim1_up.Init.Priority = DMA_PRIORITY_HIGH;
        hdma_tim1_up.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_tim1_up) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(htim, hdma[TIM_DMA_ID_UPDATE], hdma_tim1_up);
    } else if (htim->Instance == TIM8) {
        __HAL_RCC_TIM8_CLK_ENABLE();
    }
}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        __HAL_RCC_TIM1_CLK_DISABLE();
        HAL_DMA_DeInit(htim->hdma[TIM_DMA_ID_UPDATE]);
    } else if (htim->Instance == TIM8) {
        __HAL_RCC_TIM8_CLK_DISABLE();
    }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (htim->Instance == TIM1) {
        __HAL_RCC_GPIOE_CLK_ENABLE();

        GPIO_InitStruct.Pin = SIG_A_Pin | SIG_A_INV_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    } else if (htim->Instance == TIM8) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = SIG_B_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;
        HAL_GPIO_Init(SIG_B_GPIO_Port, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = SIG_B_INV_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;
        HAL_GPIO_Init(SIG_B_INV_GPIO_Port, &GPIO_InitStruct);
    }
}
