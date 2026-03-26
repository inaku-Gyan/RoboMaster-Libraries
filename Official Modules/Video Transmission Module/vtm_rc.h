/**
 * @brief VTM Remote Control Protocol Definitions
 * （裁判系统）相机图传模块协议定义
 *
 * VTM: Vision Transmission Module
 * RC: Remote Control
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 图传模块（VTM）遥控（RC）数据（解析后）
 */
typedef struct
{
    /// @brief 右摇杆 水平位置 [-660, 660] 向右为正
    int16_t stickR_h;
    /// @brief 右摇杆 竖直位置 [-660, 660] 向前为正
    int16_t stickR_v;
    /// @brief 左摇杆 竖直位置 [-660, 660] 向前为正
    int16_t stickL_v;
    /// @brief 左摇杆 水平位置 [-660, 660] 向右为正
    int16_t stickL_h;
    /// @brief 拨轮位置 [-660, 660] 向右为正
    int16_t wheel;

    /// @brief 挡位切换开关 { C, N, S }
    enum : uint8_t {
        MODE_C = 0,
        MODE_N = 1,
        MODE_S = 2
    } mode_sw;

    /// @brief 暂停键 { 0, 1 }
    uint8_t pause : 1;
    /// @brief 自定义按键（左） { 0, 1 }
    uint8_t fn_L : 1;
    /// @brief 自定义按键（右） { 0, 1 }
    uint8_t fn_R : 1;
    /// @brief 扳机键 { 0, 1 }
    uint8_t trigger : 1;

    /// @brief 鼠标左键 { 0, 1 }
    uint8_t mouse_left : 1;
    /// @brief 鼠标右键 { 0, 1 }
    uint8_t mouse_right : 1;
    /// @brief 鼠标中键 { 0, 1 }
    uint8_t mouse_middle : 1;

    /// @brief 鼠标左右移速 [-32768, 32767] 向右为正
    int16_t mouse_x;
    /// @brief 鼠标前后移速 [-32768, 32767] 向前为正
    int16_t mouse_y;
    /// @brief 鼠标滚轮速度 [-32768, 32767] 向前为正
    int16_t mouse_z;

    /**
     * @brief 键盘按键信息
     */
    struct
    {
        uint16_t W : 1;
        uint16_t S : 1;
        uint16_t A : 1;
        uint16_t D : 1;
        uint16_t Shift : 1;
        uint16_t Ctrl : 1;
        uint16_t Q : 1;
        uint16_t E : 1;
        uint16_t R : 1;
        uint16_t F : 1;
        uint16_t G : 1;
        uint16_t Z : 1;
        uint16_t X : 1;
        uint16_t C : 1;
        uint16_t V : 1;
        uint16_t B : 1;
    } key;
} vtm_rc_data_t;

/**
 * @brief 解析图传模块（VTM）遥控（RC）原始数据帧
 * @param raw_data 原始数据帧指针，必须指向一个完整的数据帧
 * @param length 原始数据帧长度，必须等于 sizeof(_raw_data_frame_t)
 * @param out 解析后的数据输出指针，必须指向一个有效的 vtm_rc_data_t 结构体
 * @return true 解析成功，数据有效；false 解析失败，数据无效
 */
bool vtm_rc_parse(const void *raw_data, size_t length, vtm_rc_data_t *out);
