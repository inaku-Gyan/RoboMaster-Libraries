#include "crc.h"
#include "stm32f4xx.h"

/* --------------------------------------------------------------------------
 * Rocksoft Model CRC configuration for each supported algorithm variant.
 * Fields: width (bits), poly, init, refIn, refOut, xorOut
 * -------------------------------------------------------------------------- */
typedef struct
{
    uint8_t width;
    uint16_t poly;
    uint16_t init;
    bool refIn;
    bool refOut;
    uint16_t xorOut;
} CrcConfig;

static const CrcConfig S_config[CRC_TYPE_COUNT] = {
    [CRC_8_MAXIM_INIT_FF] = {.width = 8, .poly = 0x0031, .init = 0x00FF, .refIn = true, .refOut = true, .xorOut = 0x0000},
    [CRC_16_MCRF4XX]      = {.width = 16, .poly = 0x1021, .init = 0xFFFF, .refIn = true, .refOut = true, .xorOut = 0x0000},
};

// 初始化时，预计算每种 CRC 类型的 256 条单字节输入的 CRC 转换表，以加速后续的计算。
static uint16_t S_lookup[CRC_TYPE_COUNT][256];

#define LOOKUP S_lookup[type]

// 通过检查预计算表中的几个特定条目来判断是否已经初始化过了
#define IS_INITIALIZED(type) (LOOKUP[1] != 0 || LOOKUP[128] != 0)

/* Reverse the lower 'width' bits of val. */
static uint16_t reflect_bits(uint16_t val, uint8_t width)
{
    uint16_t result = 0;
    for (uint8_t i = 0; i < width; i++) {
        if (val & (1u << i)) {
            result |= (uint16_t)(1u << (width - 1u - i));
        }
    }
    return result;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void crc_init(CrcType type)
{
    assert_param(type < CRC_TYPE_COUNT);
    assert_param(!IS_INITIALIZED(type)); // 避免重复初始化

    if (IS_INITIALIZED(type))
        return;

    const CrcConfig *cfg = &S_config[type];
    const uint16_t mask  = (cfg->width == 8u) ? 0x00FFu : 0xFFFFu;

    if (cfg->refIn) {
        /* Reflected (LSB-first) table: use the bit-reversed polynomial. */
        const uint16_t rpoly = reflect_bits(cfg->poly, cfg->width);
        for (uint16_t i = 0; i < 256u; i++) {
            uint16_t crc = i;
            for (uint8_t j = 0; j < 8u; j++) {
                crc = (crc & 1u) ? ((crc >> 1) ^ rpoly) : (crc >> 1);
            }
            LOOKUP[i] = crc;
        }
    } else {
        /* Normal (MSB-first) table. */
        const uint8_t shift    = (uint8_t)(cfg->width - 8u);
        const uint16_t top_bit = (uint16_t)(1u << (cfg->width - 1u));
        for (uint16_t i = 0; i < 256u; i++) {
            uint16_t crc = (uint16_t)(i << shift);
            for (uint8_t j = 0; j < 8u; j++) {
                crc = (crc & top_bit)
                          ? (uint16_t)((crc << 1) ^ cfg->poly)
                          : (uint16_t)(crc << 1);
            }
            LOOKUP[i] = crc & mask;
        }
    }
}

uint16_t crc_calculate(CrcType type, const uint8_t *data, size_t length)
{
    assert_param(data != NULL || length == 0);
    assert_param(type < CRC_TYPE_COUNT);
    assert_param(IS_INITIALIZED(type)); // 确保已经初始化过了

    const CrcConfig *cfg = &S_config[type];
    const uint16_t mask  = (cfg->width == 8u) ? 0x00FFu : 0xFFFFu;
    uint16_t crc         = cfg->init;

    for (size_t i = 0; i < length; i++) {
        if (cfg->refIn) {
            if (cfg->width == 16u) {
                /* Reflected 16-bit: new byte enters from the LSB side. */
                crc = (uint16_t)((crc >> 8) ^ LOOKUP[(crc ^ data[i]) & 0xFFu]);
            } else {
                /* Reflected 8-bit. */
                crc = LOOKUP[(crc ^ data[i]) & 0xFFu];
            }
        } else {
            if (cfg->width == 16u) {
                /* Normal 16-bit: new byte enters from the MSB side. */
                crc = (uint16_t)(LOOKUP[((crc >> 8) ^ data[i]) & 0xFFu] ^ (crc << 8));
            } else {
                /* Normal 8-bit. */
                crc = LOOKUP[(crc ^ data[i]) & 0xFFu];
            }
        }
    }

    /* Handle the (rare) case where output reflection differs from input. */
    if (cfg->refOut != cfg->refIn) {
        crc = reflect_bits(crc, cfg->width);
    }

    return (uint16_t)((crc ^ cfg->xorOut) & mask);
}

bool crc_verify(CrcType type, const uint8_t *data, size_t length)
{
    assert_param(data != NULL);
    assert_param(type < CRC_TYPE_COUNT);
    assert_param(IS_INITIALIZED(type));

    if (data == NULL) {
        return false;
    }

    const CrcConfig *cfg = &S_config[type];
    const size_t crc_len = cfg->width / 8u;

    if (length <= crc_len) {
        return false;
    }

    const uint16_t computed = crc_calculate(type, data, length - crc_len);

    if (crc_len == 1u) {
        return (uint8_t)computed == data[length - 1u];
    } else {
        /* CRC-16 stored little-endian (LSB at lower address). */
        return ((uint8_t)(computed & 0xFFu) == data[length - 2u]) && ((uint8_t)((computed >> 8) & 0xFFu) == data[length - 1u]);
    }
}

void crc_append(CrcType type, uint8_t *data, size_t length)
{
    assert_param(data != NULL);
    assert_param(type < CRC_TYPE_COUNT);
    assert_param(IS_INITIALIZED(type));

    if (data == NULL) {
        return;
    }

    const CrcConfig *cfg = &S_config[type];
    const size_t crc_len = cfg->width / 8u;

    if (length <= crc_len) {
        return;
    }

    const uint16_t crc = crc_calculate(type, data, length - crc_len);

    if (crc_len == 1u) {
        data[length - 1u] = (uint8_t)crc;
    } else {
        /* CRC-16 stored little-endian (LSB at lower address). */
        data[length - 2u] = (uint8_t)(crc & 0xFFu);
        data[length - 1u] = (uint8_t)((crc >> 8) & 0xFFu);
    }
}

#undef LOOKUP
#undef IS_INITIALIZED
