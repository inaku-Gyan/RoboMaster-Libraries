#include "hal_utils.h"

uint32_t halUtils_TIM_GetClockFreq_Hz(const TIM_TypeDef *const tim)
{
    assert_param(IS_TIM_INSTANCE(tim));

    uint32_t pclk, clk_div;

    // 获取当前时钟配置

    RCC_ClkInitTypeDef RCC_ClkConfig = {0}; // RCC 时钟配置结构体
    uint32_t _                       = 0;   // Dummy variable
    HAL_RCC_GetClockConfig(&RCC_ClkConfig, &_);

    // 判断定时器属于 APB1 还是 APB2
    if (halUtils_IS_APB1_PERIPH(tim)) {
        /* === APB1 总线 (TIM 2, 3, 4, 5, 6, 7, 12, 13, 14) === */
        pclk    = HAL_RCC_GetPCLK1Freq(); // 返回值单位 Hz
        clk_div = RCC_ClkConfig.APB1CLKDivider;
    } else {
        /* === APB2 总线 (TIM 1, 8, 9, 10, 11) === */
        pclk    = HAL_RCC_GetPCLK2Freq();
        clk_div = RCC_ClkConfig.APB2CLKDivider;
    }
    // 检查 APB 预分频系数
    // 如果 APB 分频系数为 1，则倍频 = 1，否则倍频 = 2
    if (clk_div == RCC_HCLK_DIV1)
        return pclk;
    return pclk * 2;
}

void halUtils_TIM_SetDutyCycle(TIM_HandleTypeDef *htim, uint32_t Channel, float duty)
{
    if (duty < 0.0f) {
        duty = 0.0f;
    } else if (duty > 1.0f) {
        duty = 1.0f;
    }

    uint32_t period = __HAL_TIM_GET_AUTORELOAD(htim);
    uint32_t pulse  = (uint32_t)((float)period * duty);

    __HAL_TIM_SET_COMPARE(htim, Channel, pulse);
}

void halUtils_TIM_SetPulseWidth_us(TIM_HandleTypeDef *htim, uint32_t Channel, float width_us)
{
    uint32_t tim_clock = halUtils_TIM_GetClockFreq_Hz(htim->Instance);
    uint32_t prescaler = htim->Instance->PSC;
    float freq         = (float)tim_clock / (float)(prescaler + 1);

    // count = width_us * 1us * freq = width_us * freq / 1000000.0f
    uint32_t pulse = (uint32_t)(width_us * freq / 1000000.0f);

    uint32_t period = __HAL_TIM_GET_AUTORELOAD(htim);
    if (pulse > period) {
        pulse = period;
    }

    __HAL_TIM_SET_COMPARE(htim, Channel, pulse);
}
