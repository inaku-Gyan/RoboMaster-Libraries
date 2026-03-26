#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Supported CRC algorithm variants described by the Rocksoft Model parameters:
 *   (width, poly, init, refIn, refOut, xorOut)
 *
 * T = true (reflected), F = false (normal/MSB-first)
 */
typedef enum {
    /**
     * @brief 非标准的 CRC-8/MAXIM 算法，初始值改为了 0xFF 而非 0x00.
     *
     * (8, 0x31, 0xFF, T, T, 0x00)
     * @note 用于 Dji RoboMaster 裁判系统串口通信协议的 CRC-8 帧头校验
     */
    CRC_8_MAXIM_INIT_FF = 0,

    /**
     * @brief 标准 CRC-16/MCRF4XX 算法。
     *
     * (16, 0x1021, 0xFFFF, T, T, 0x0000)
     * @note 用于 Dji RoboMaster 裁判系统串口通信协议的 CRC-16 整包校验；也用于 VTM 图传模块 RC 数据的 CRC-16 校验
     */
    CRC_16_MCRF4XX,

    // The total count of CRC types
    CRC_TYPE_COUNT
} CrcType;

/**
 * Initialize the internal CRC lookup table for the given type.
 * Must be called before using crc_calculate / crc_verify / crc_append.
 */
void crc_init(CrcType type);

/**
 * Compute the CRC of a data buffer.
 * @param type   The CRC type must match one of the types used in crc_init.
 * @param data   Input data pointer.
 * @param length Number of data bytes.
 * @return       CRC value (lower 8 bits for CRC-8, full 16 bits for CRC-16).
 */
uint16_t crc_calculate(CrcType type, const uint8_t *data, size_t length);

/**
 * Verify the CRC bytes appended at the end of a buffer.
 * CRC-16 bytes are expected in little-endian order (LSB at lower address).
 * @param type   The CRC type must match one of the types used in crc_init.
 * @param data   Buffer containing data followed by CRC byte(s).
 * @param length Total buffer size including the CRC byte(s).
 * @return       true if the CRC is valid.
 */
bool crc_verify(CrcType type, const uint8_t *data, size_t length);

/**
 * Compute and write the CRC byte(s) into the reserved space at the end of a buffer.
 * CRC-16 bytes are written in little-endian order (LSB at lower address).
 * @param type   The CRC type must match one of the types used in crc_init.
 * @param data   Buffer containing data followed by reserved space for CRC byte(s).
 * @param length Total buffer size including the reserved CRC byte(s).
 */
void crc_append(CrcType type, uint8_t *data, size_t length);
