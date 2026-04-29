#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <cstdint> //提供了定义了几个整数类型的头文件，如int8_t、uint32_t等。
#include <vector> //提供了定义了std::vector容器的头文件，std::vector是一个动态数组，可以自动调整大小。

class FixedSizePool // 固定大小内存池（每个小块内存的大小是固定的）
{
public:
    explicit FixedSizePool(std::size_t block_size, std::size_t blocks_per_page = 1024); //内存池构造函数，explicit关键字表示该构造函数不能被隐式调用，必须显式调用。
    ~FixedSizePool(); //内存池析构函数，负责释放所有已经分配的内存页，避免内存泄漏

    void* allocate(); // 申请小块内存
    void deallocate(void* p); // 释放小块内存

    std::size_t block_size() const { return block_size_; } // 获取每个小块内存的大小
    std::size_t blocks_per_page() const { return blocks_per_page_; } // 获取每页内存中小块内存的数量

private:
    struct Node
    {
        Node* next; //指向下一个节点的指针
    };

    void expand();
    std::size_t adjust_block_size(std::size_t s);//调整小块内存的大小，使其满足对齐要求，通常是8字节对齐，以提高内存访问效率。
    static inline std::size_t align_up(std::size_t n, std::size_t align) //对齐函数，将n向上对齐到align的倍数   
    {
        return (n + (align - 1)) & ~(align - 1); //将n加上align-1，然后与~(align-1)进行按位与运算，得到向上对齐后的结果
     }
    std::size_t block_size_; //每个小块内存的大小
    std::size_t blocks_per_page_; //每页内存中小块内存的数量
    Node* free_list_; //小块内存的链表头指针，指向当前可用的小块内存(空闲小块的单向链表头)
    std::vector<void*> pages_; //维护大块内存的列表，每个元素指向一个大块内存 (所有已经分配的页，析构统一释放)
};



#endif