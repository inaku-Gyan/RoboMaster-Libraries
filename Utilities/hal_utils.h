#pragma once
#include "stm32f4xx_hal.h"

/** @addtogroup 判断外设所属总线
 * @{
 * 原理参考 stm32f407xx.h
 */

/** 判断外设是否属于 APB1 */
#define halUtils_IS_APB1_PERIPH(periph) ((void *)APB1PERIPH_BASE <= (void *)(periph) && (void *)(periph) < (void *)APB2PERIPH_BASE)
/** 判断外设是否属于 APB2 */
#define halUtils_IS_APB2_PERIPH(periph) ((void *)APB2PERIPH_BASE <= (void *)(periph) && (void *)(periph) < (void *)AHB1PERIPH_BASE)
/** 判断外设是否属于 AHB1 */
#define halUtils_IS_AHB1_PERIPH(periph) ((void *)AHB1PERIPH_BASE <= (void *)(periph) && (void *)(periph) < (void *)AHB2PERIPH_BASE)
/** 判断外设是否属于 AHB2 */
#define halUtils_IS_AHB2_PERIPH(periph) ((void *)AHB2PERIPH_BASE <= (void *)(periph))

/**
 * @}
 */

/**
 * @brief  获取指定定时器的输入时钟频率
 * @param[in]  htim: 定时器句柄指针 (例如 &htim4)
 * @return 定时器时钟频率 (Hz)
 */
uint32_t halUtils_TIM_GetClockFreq_Hz(const TIM_TypeDef *const tim);

/**
 * @brief 设置指定的 TIM Channel 的占空比
 * @param[in] htim 定时器句柄 (如 &htim8)
 * @param[in] Channel 输出通道 (如 TIM_CHANNEL_3)
 * @param[in] duty 占空比, 范围 0.0f ~ 1.0f
 */
void halUtils_TIM_SetDutyCycle(TIM_HandleTypeDef *htim, uint32_t Channel, float duty);

/**
 * @brief 设置指定的 TIM Channel 的脉冲宽度（时间长度）
 * @param[in] htim 定时器句柄 (如 &htim8)
 * @param[in] Channel 输出通道 (如 TIM_CHANNEL_3)
 * @param[in] width_us 脉冲宽度，单位微秒 (us)
 */
void halUtils_TIM_SetPulseWidth_us(TIM_HandleTypeDef *htim, uint32_t Channel, float width_us);
