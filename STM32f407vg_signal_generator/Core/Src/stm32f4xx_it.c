#include "main.h"
#include "stm32f4xx_it.h"

extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_tim1_up;

void NMI_Handler(void)
{
    while (1) {
    }
}

void HardFault_Handler(void)
{
    while (1) {
    }
}

void MemManage_Handler(void)
{
    while (1) {
    }
}

void BusFault_Handler(void)
{
    while (1) {
    }
}

void UsageFault_Handler(void)
{
    while (1) {
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void DMA1_Stream4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_spi2_tx);
}

void DMA2_Stream5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_tim1_up);
}
