#pragma once
#include "cmsis_os2.h"

/**
 * @brief 将毫秒时间转换为 RTOS 的系统节拍数
 */
#define osUtil_ms_to_ticks(ms) ((uint32_t)((ms) * osKernelGetTickFreq() / 1000U))

/**
 * @brief 将系统节拍数转换为毫秒时间
 */
#define osUtil_ticks_to_ms(ticks) ((uint32_t)((ticks) * 1000U / osKernelGetTickFreq()))
