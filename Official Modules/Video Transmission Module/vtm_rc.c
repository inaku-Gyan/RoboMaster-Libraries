#include "vtm_rc.h"
#include "crc.h"  // 需要实现 CRC-16/MCRF4XX 的 crc_verify 函数
#include "stm32f4xx_hal.h"

/// @brief 图传模块（VTM）遥控（RC）原始数据帧定义
typedef struct __packed {
    uint8_t sof_1; // 固定帧头 0xA9
    uint8_t sof_2; // 固定帧头 0x53

    uint64_t ch_0 : 11;   // 右摇杆水平方向位置. 取值: [364, 1684]，中位 1024
    uint64_t ch_1 : 11;   // 右摇杆竖直方向位置. 取值: [364, 1684]，中位 1024
    uint64_t ch_2 : 11;   // 左摇杆竖直方向位置. 取值: [364, 1684]，中位 1024
    uint64_t ch_3 : 11;   // 左摇杆水平方向位置. 取值: [364, 1684]，中位 1024
    uint64_t mode_sw : 2; // 挡位切换开关位置. 取值: C: 0, N: 1, S: 2
    uint64_t pause : 1;   // 暂停键. 按下: 1, 未按下: 0
    uint64_t fn_1 : 1;    // 自定义按键（左）. 按下: 1, 未按下: 0
    uint64_t fn_2 : 1;    // 自定义按键（右）. 按下: 1, 未按下: 0
    uint64_t wheel : 11;  // 拨轮位置. 取值: [364, 1684]，中位 1024
    uint64_t trigger : 1; // 扳机键. 按下: 1, 未按下: 0

    int16_t mouse_x;          // 鼠标左右移动的速度（负值向左）. 取值范围: [-32768, 32767]
    int16_t mouse_y;          // 鼠标前后移动的速度（负值向后）. 取值范围: [-32768, 32767]
    int16_t mouse_z;          // 鼠标滚轮的滚动速度（负值向后）. 取值范围: [-32768, 32767]
    uint8_t mouse_left : 2;   // 鼠标左键. 按下: 1, 未按下: 0
    uint8_t mouse_right : 2;  // 鼠标右键. 按下: 1, 未按下: 0
    uint8_t mouse_middle : 2; // 鼠标中键. 按下: 1, 未按下: 0
    uint16_t key;             // 键盘按键信息，每个 bit 代表一个键，1 表示按下，0 表示未按下
    uint16_t crc16;
} _raw_data_frame_t;

_Static_assert(sizeof(_raw_data_frame_t) == 21, "VTM RC data frame size mismatch");

static inline void _parse_raw_data(const _raw_data_frame_t *raw, vtm_rc_data_t *out)
{
    out->stickR_h = (int16_t)raw->ch_0 - 1024;
    out->stickR_v = (int16_t)raw->ch_1 - 1024;
    out->stickL_v = (int16_t)raw->ch_2 - 1024;
    out->stickL_h = (int16_t)raw->ch_3 - 1024;

    out->wheel = (int16_t)raw->wheel - 1024;

    out->mode_sw = raw->mode_sw;
    out->pause   = raw->pause;
    out->fn_L    = raw->fn_1;
    out->fn_R    = raw->fn_2;
    out->trigger = raw->trigger;

    out->mouse_x      = raw->mouse_x;
    out->mouse_y      = raw->mouse_y;
    out->mouse_z      = raw->mouse_z;
    out->mouse_left   = raw->mouse_left;
    out->mouse_right  = raw->mouse_right;
    out->mouse_middle = raw->mouse_middle;

    *((uint16_t *)&out->key) = raw->key;
}

static inline bool _validate_raw_data(const void *raw_data, size_t length)
{
    if (length != sizeof(_raw_data_frame_t))
        return false; // 数据长度不正确

    const _raw_data_frame_t *frame = (const _raw_data_frame_t *)raw_data;

    if (frame->sof_1 != (uint8_t)0xA9 || frame->sof_2 != (uint8_t)0x53)
        return false; // 帧头不正确

    return crc_verify(CRC_16_MCRF4XX, raw_data, length);
}

bool vtm_rc_parse(const void *raw_data, size_t length, vtm_rc_data_t *out)
{
    if (raw_data == NULL || out == NULL)
        return false; // 参数无效

    if (!_validate_raw_data(raw_data, length))
        return false; // 数据无效

    _parse_raw_data(raw_data, out);
    return true; // 解析成功，数据有效
}
