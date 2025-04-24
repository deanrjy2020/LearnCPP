#include <cassert>
#include <iostream>
using namespace std;

namespace mem_addr_align {

//=========================================================
// mem_addr_align

// void* aligned_malloc (size_t size, size_t align)
//
// allocated buffer using standard C malloc and return the size of the
// requested buffer which satisfies requested memory pointer alignment
//
// return value: user accessible buffer pointer
// size: requested user accessible buffer
// align: byte size of the alignment of the returned memory pointer
//
// 思路:
// malloc分配的一般已经是16B align了, 假设是没有对齐的, 任意地址, 以16对齐为例
// 64bit机器.
// 1,
// 最多要移动多少单位? align-1单位
// 以size=2014, align=16为例, 假设有一块很大的mem, 从0到很大(远大于1024),
// 如果malloc开出来的ptr的正好在0, 已经是对其了, 不用做另外的事情,
// 如果ptr在1, 那么下一个对其的地址在16, 要shift 15个Byte,
// 如果ptr在2, 下一个地址在16, shift 14个Byte,
// …
// 如果ptr在15, shift 1个Byte,
// Ptr在16,不用动,
// Ptr在17, 下一个地址在32, shift 15Byte,
// 所以最多移动15, 即align-1 Byte空间就可以了.
// 2,
// 下一个对齐的地址怎么找? 记住: addr = addr + (align-1) & ~(align-1);
// 举两个例子, 以后回来看的时候方便点,
// Align-1=15, 其二进制为: 0000, 1111
// 取了~后为               1111, 0000
// 例子1, 假设addr为0X00, 正好在内存0位置, 加上15后为0X0F,
// 和自己的非 &一下, 为0, 还是在原点, 即addr已经是对齐的了
// 例子2,假设addr为0x03, 加上15后为18,
// 二进制为: 0001, 0010
// &上      1111, 0000
// 等于     0001, 0000
// 等于十进制的16, 即0x10, 也就是说0x03下一个对齐的位置为16 (0x10), 向后移动了13 Byte,
// 3,
// malloc得到realPtr, 向后移动pointerSize, 得到ptr, 然后把ptr对齐, 得到userPtr
// 把addr的值放在userPtr的前面, 返回userPtr
// 要多开辟多少mem? 存realPtr要sizeof(void*)=8B大小存, 对齐最多要移动15B,
// 所以maxShift = sizeof(void*) + (align-1) = 8 + 15
// 所以要分配的内存总大小totalMemSize = userSize + maxShift;
// free的时候拿到ptr, 得到前面隐藏的realPtr值, 就是真正要free的内存.

// 测试用
class Debug {
private:
    void* realPtr;
    void* userPtr;

public:
    void save(void* realPtr, void* userPtr) {
        this->realPtr = realPtr;
        this->userPtr = userPtr;
    }
    bool verify(void* realPtr, void* userPtr) {
        bool res = this->realPtr == realPtr && this->userPtr == userPtr;

        // clear after each verify()
        this->realPtr = nullptr;
        this->userPtr = nullptr;

        return res;
    }
};
Debug dbg;

void* aligned_malloc(size_t size, size_t align) {
    // 分配足够的内存, 这里的算法很经典, 早期的STL中使用的就是这个算法

    // 考虑到mem size也要是align的倍数
    if (size % align) {
        return nullptr;
    }

    // 首先是放真正的ptr指针占用的内存大小
    const size_t pointerSize = sizeof(void*);  // 8B

    // 要分配的内存总大小
    size_t totalMemSize = size + pointerSize + align - 1;

    // 分配
    void* realPtr = malloc(totalMemSize);
    if (!realPtr) {
        return nullptr;
    }

    // 先移动一些, 确保前面有空间放真正的内存地址
    uintptr_t ptr = (uintptr_t)realPtr + pointerSize;
    // 对齐, 有可能不会移动, 如果ptr已经是对齐的话.
    ptr = (ptr + align - 1) & ~(align - 1);
    // userPtr就是要返回给用户用的地址
    void* userPtr = (void*)ptr;

    // 把真正的地址藏起来.
    void* preUserPtr = (void*)(ptr - pointerSize);
    *(void**)preUserPtr = realPtr;

    // 测试, 保存起来
    dbg.save(realPtr, userPtr);

    return userPtr;
}

//
// void aligned_free(void* ptr)
//
// return the memory to the system using standard C free function.
// return value : void
void aligned_free(void* ptr) {
    const size_t pointerSize = sizeof(void*);  // 8B
    uintptr_t userPtr = (uintptr_t)ptr;
    void* preUserPtr = (void*)(userPtr - pointerSize);
    void* realPtr = *(void**)preUserPtr;

    // 测试
    assert(dbg.verify(realPtr, ptr));

    free(realPtr);
}

bool isAligned(void* ptr, size_t align) {
    return ((uintptr_t)ptr & (align - 1)) == 0;
}

int cppMain() {
    {
        void* p = malloc(4);
        assert(isAligned(p, 16));  // 默认的malloc是16 对齐的.
        cout << "malloc, p = " << p << endl;
        free(p);
    }

    size_t align = 32;
    {
        // size不是对齐的, 不干活
        void* p = aligned_malloc(65, align);
        assert(p == nullptr);
    }

    {
        // 干活, 按要求对齐.
        void* p = aligned_malloc(64, align);
        assert(isAligned(p, align));
        cout << "aligned_malloc p = " << p << endl;
        aligned_free(p);
    }

    return 0;
}

}  // namespace mem_addr_align

/*===== Output =====

[RUN  ] mem_addr_align
malloc, p = 0x216f55f7df0
aligned_malloc p = 0x216f55f6160
[tid=1] [Memory Report] globalNewCnt = 0, globalDeleteCnt = 0, globalNewMemSize = 0, globalDeleteMemSize = 0
[   OK] mem_addr_align

*/
