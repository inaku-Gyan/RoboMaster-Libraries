#include "bufPipe.h"
#include "stm32f4xx_hal.h"

typedef struct
{
    size_t flag : 16; // 用于检测内存越界等问题的标志值
    size_t size : 16; // 缓冲区块的大小（字节）
    uint8_t buf[];    // 缓冲区块
} Node;

// 标志值，表示块正在被写入中（尚未提交）
#define _FlagWriting ((size_t)0x50CA)
// 标志值，表示块已在队列中（已提交）
#define _FlagInQueue ((size_t)0xC0A1)
// 标志值，表示块正在被读取中（尚未释放）
#define _FlagReading ((size_t)0xA0E1)
// 标志值，表示块已释放（仅用于调试，实际释放后内存块不可访问）
#define _FlagReleased ((size_t)0xDEAD) // 内存已释放（仅用于调试，实际释放后内存块不可访问）

typedef Node *NodePtr;

/// @brief 计算实际分配的缓冲区块大小（字节），包括 Node 结构体的开销
#define nodeSize(bufSize) (sizeof(Node) + (bufSize))

#define getBufSize(pipe) (osMemoryPoolGetBlockSize((pipe)->memPool) - sizeof(Node))

#define toNodePtr(ptr) ((NodePtr)((uint8_t *)(ptr) - offsetof(Node, buf)))

static inline NodePtr allocNode(BufPipe_t *pipe, uint32_t timeout)
{
    return osMemoryPoolAlloc(pipe->memPool, timeout);
}

static inline NodePtr readNode(BufPipe_t *pipe, uint32_t timeout)
{
    NodePtr pNode = NULL;
    if (osMessageQueueGet(pipe->writtenNodeQ, &pNode, NULL, timeout) != osOK)
        return NULL;
    return pNode;
}

//// ---- Public APIs ---- ////

bool BufPipe_Init(BufPipe_t *pipe, size_t bufCount, size_t bufSize)
{
    assert_param(pipe != NULL);
    if (pipe == NULL || bufCount == 0 || bufSize == 0)
        return false;
    if (pipe->memPool != NULL || pipe->writtenNodeQ != NULL)
        return false; // 已初始化的管道不允许重复初始化

    pipe->writtenNodeQ = osMessageQueueNew(bufCount, sizeof(NodePtr), NULL);
    if (pipe->writtenNodeQ == NULL)
        return false;

    pipe->memPool = osMemoryPoolNew(bufCount, nodeSize(bufSize), NULL);
    if (pipe->memPool == NULL) {
        osMessageQueueDelete(pipe->writtenNodeQ);
        pipe->writtenNodeQ = NULL;
        return false;
    }
    return true;
}

void *BufPipe_Alloc(BufPipe_t *pipe, uint32_t timeout)
{
    if (pipe == NULL)
        return NULL;

    NodePtr pNode = allocNode(pipe, timeout);
    if (pNode != NULL) {
        pNode->flag = _FlagWriting; // 写入标志值以检测内存越界等问题
        pNode->size = 0;            // 初始数据长度为 0
        return pNode->buf;
    }
    return NULL;
}

void *BufPipe_AllocOverwrite(BufPipe_t *pipe, uint32_t timeout)
{
    assert_param(timeout != 0U); // 覆写模式不支持在 ISR 中调用，因为无法实现预期的覆写行为
    if (pipe == NULL)
        return NULL;

    // 先尝试直接从内存池非阻塞分配
    NodePtr pNode = allocNode(pipe, 0U);
    if (pNode == NULL) {
        // 内存池已耗尽：弹出最旧的已入队块（若队列为空则无法覆写，返回 NULL）
        pNode = readNode(pipe, timeout);
    }

    if (pNode != NULL) {
        pNode->flag = _FlagWriting; // 写入标志值以检测内存越界等问题
        pNode->size = 0;            // 初始数据长度为 0
        return pNode->buf;
    }
    return NULL;
}

bool BufPipe_Send(BufPipe_t *pipe, void *buf, size_t writtenSize, uint32_t timeout)
{
    if (pipe == NULL || buf == NULL)
        return false;

    const NodePtr pNode = toNodePtr(buf);

    assert_param(pNode->flag == _FlagWriting); // 检测内存越界等问题
    if (pNode->flag != _FlagWriting)
        return false; // 内存损坏，拒绝发送

    if (writtenSize > getBufSize(pipe)) {
        // 写入大小超过缓冲区块的容量，拒绝发送
        return false;
    }

    pNode->flag = _FlagInQueue; // 入队标志值以检测内存越界等问题
    pNode->size = writtenSize;
    return osMessageQueuePut(pipe->writtenNodeQ, &pNode, 0, timeout) == osOK;
}

void *BufPipe_Recv(BufPipe_t *pipe, uint32_t timeout)
{
    if (pipe == NULL)
        return NULL;

    const NodePtr pNode = readNode(pipe, timeout);
    if (pNode == NULL)
        return NULL;

    // 读取标志值以检测内存越界等问题
    assert_param(pNode->flag == _FlagInQueue);

    pNode->flag = _FlagReading;
    return pNode->buf;
}

size_t BufPipe_DataLen(const void *buf)
{
    assert_param(buf != NULL);
    if (buf == NULL)
        return 0U;

    const NodePtr pNode = toNodePtr(buf);

    // 检测内存越界等问题
    assert_param(pNode->flag == _FlagInQueue || pNode->flag == _FlagReading);
    if (pNode->flag != _FlagInQueue && pNode->flag != _FlagReading)
        return 0U; // 内存损坏，无法获取数据长度

    return pNode->size;
}

bool BufPipe_Release(BufPipe_t *pipe, void *buf)
{
    if (pipe == NULL || buf == NULL)
        return false;

    const NodePtr pNode = toNodePtr(buf);

    assert_param(pNode->flag == _FlagWriting || pNode->flag == _FlagReading); // 检测内存越界等问题
    if (pNode->flag != _FlagWriting && pNode->flag != _FlagReading)
        return false; // 内存损坏，拒绝释放

    pNode->flag = _FlagReleased; // 标记为已释放
    return osMemoryPoolFree(pipe->memPool, pNode) == osOK;
}
