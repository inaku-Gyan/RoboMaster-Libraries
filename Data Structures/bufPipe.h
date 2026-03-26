#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os2.h"

/**
 * @brief 基于 CMSIS-RTOS2 实现的零拷贝缓冲区管道（Buffer Pipe）
 *
 * 生产者通过 BufPipe_Alloc 获得缓冲区所有权，写入数据后调用 BufPipe_Send 将所有权
 * 转移给消费者；消费者通过 BufPipe_Recv 接管所有权，读取完毕后调用 BufPipe_Release 归还。
 */
typedef struct {
    osMemoryPoolId_t memPool;        // 用于分配缓冲区块的内存池
    osMessageQueueId_t writtenNodeQ; // 传递已写入缓冲区块指针的消息队列. 数据类型: NodePtr
} BufPipe_t;

/**
 * @brief 初始化缓冲区管道
 * @param pipe 指向 BufPipe_t 的指针
 * @param bufCount 缓冲区块数量
 * @param bufSize  缓冲区块大小（字节）
 * @return 初始化成功返回 true，否则返回 false
 */
bool BufPipe_Init(BufPipe_t *pipe, size_t bufCount, size_t bufSize);

/**
 * @brief 从内存池分配一个缓冲区块（生产者调用）
 * @param pipe    指向 BufPipe_t 的指针
 * @param timeout 等待空闲块的超时时间（CMSIS-RTOS2 时间单位）
 * @return 成功返回缓冲区指针，内存池耗尽或超时返回 NULL
 */
void *BufPipe_Alloc(BufPipe_t *pipe, uint32_t timeout);

/**
 * @brief 从内存池分配一个缓冲区块；若内存池耗尽则丢弃最旧的未读块以腾出空间（覆写模式）
 * @param pipe 指向 BufPipe_t 的指针
 * @param timeout 等待空闲块或可覆写块的超时时间（CMSIS-RTOS2 时间单位）
 * @return 成功返回缓冲区指针；若所有块均被消费者持有（队列为空且内存池耗尽）则返回 NULL
 * @note  **不支持在 ISR 中调用**
 *        因为 RTOS 会延迟执行 ISR 中被调用的 API，所以预期的覆写行为无法实现。
 */
void *BufPipe_AllocOverwrite(BufPipe_t *pipe, uint32_t timeout);

/**
 * @brief 将已写入数据的缓冲区块发送给消费者（转移所有权）
 * @param pipe        指向 BufPipe_t 的指针
 * @param buf         由 BufPipe_Alloc 返回的缓冲区指针
 * @param writtenSize 已写入的数据长度（字节）
 * @param timeout     等待队列空位的超时时间（CMSIS-RTOS2 时间单位）
 * @return 成功返回 true，否则返回 false
 */
bool BufPipe_Send(BufPipe_t *pipe, void *buf, size_t writtenSize, uint32_t timeout);

/**
 * @brief 接收一个缓冲区块（消费者调用，接管所有权）
 * @param pipe    指向 BufPipe_t 的指针
 * @param timeout 等待数据的超时时间（CMSIS-RTOS2 时间单位）
 * @return 成功返回缓冲区指针，超时或出错返回 NULL
 */
void *BufPipe_Recv(BufPipe_t *pipe, uint32_t timeout);

/**
 * @brief 获取缓冲区中已写入的数据长度
 * @param buf 由 BufPipe_Recv 返回的缓冲区指针
 * @return 数据长度（字节）
 */
size_t BufPipe_DataLen(const void *buf);

/**
 * @brief 读取完毕后释放缓冲区块，将所有权归还给内存池
 * @param buf 由 BufPipe_Recv 返回的缓冲区指针
 * @return 成功返回 true，否则返回 false
 */
bool BufPipe_Release(BufPipe_t *pipe, void *buf);

/**
 * @brief 获取当前管道中可供消费者读取的缓冲区块数量
 * @param pipe 指向 BufPipe_t 的指针
 * @return 缓冲区块数量
 */
static inline size_t BufPipe_DataCount(BufPipe_t *pipe)
{
    if (pipe == NULL || pipe->writtenNodeQ == NULL)
        return 0U;
    return osMessageQueueGetCount(pipe->writtenNodeQ);
}
