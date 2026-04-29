#include "MemoryPool.h" //包含了自定义的内存池类的头文件，提供了内存池的定义和接口。
#include <new> //提供了定义了几个与内存分配相关的函数和类型的头文件，如operator new、std::nothrow等。
#include <cstddef> //提供了定义了几个类型和宏的头文件，如size_t、ptrdiff_t、NULL等。
#include <cassert> //提供了定义了assert宏的头文件，assert宏用于在调试阶段检查程序的假设，如果条件不满足则会终止程序并输出错误信息。
//#include <vector> //提供了定义了std::vector容器的头文件，std::vector是一个动态数组，可以自动调整大小。
//#include <cstdint> //提供了定义了几个整数类型的头文件，如int8_t、uint32_t等。
//#include <iostream> //提供了定义了输入输出流的头文件，如std::cout、std::cin等，用于进行输入输出操作。
//#include <iomanip> //用于格式化输出
//#include <chrono> // 专门用来计时的库


//  单个大块内存（页） [大块内存] -> 切分 -> [小块1] [小块2] [小块3]......[小块n] ; 申请大块内存的操作一次,后面的小块内存都不是new出来的，而是从大块内存中切分出来的;
//                                  ⬆                        ↓
//                           allocate申请小块内存         deallocate释放小块内存
//  单个大块内存（页） [大块内存] -> 切分 -> [小块1] [小块2] [小块3]......[小块n] ;


    FixedSizePool::FixedSizePool(std::size_t block_size, std::size_t blocks_per_page) 
    {
        blocks_per_page_ = blocks_per_page; //每页内存中小块内存的数量
        block_size_ = adjust_block_size(block_size); //每个小块内存的大小，调用adjust_block_size函数调整小块内存的大小，使其满足对齐要求
        free_list_ = nullptr; //小块内存的链表头指针，初始化为nullptr表示当前没有可用的小块内存
    }
    FixedSizePool::~FixedSizePool()
    {
        for (void* p : pages_) //把pages_中每个元素拿出来叫p，然后执行{}中的操作
        {  
            ::operator delete(p); //释放每页内存 与delete p;不同，delete会调用对象的析构函数，而operator delete只是释放内存，不调用析构函数 
        }                         //：：的作用是告诉编译器使用全局的operator delete函数，而不是类的成员函数。因为我们在这里需要释放的是大块内存，而不是小块内存，所以需要使用全局的operator delete函数来释放内存。
    }
    //分配一小块内存
    void* FixedSizePool::allocate() // 申请小块内存
    {
        if (free_list_ == nullptr) //如果当前没有可用的小块内存 写法2： if (!free_list_)
        { 
            expand(); //申请一页内存，并把这一页内存切分成多个小块内存，并将这些小块内存加入到链表中，供后续的allocate和deallocate操作使用。
        }

        Node* head = free_list_; //将链表的头节点赋值给head，head指向当前可用的小块内存
        free_list_ = head->next; //将链表的头节点的下一个节点赋值给free_list_，更新链表的头指针，指向下一个可用的小块内存
        return head; //返回当前可用的小块内存的地址，供调用者使用
    }

    //归还一小块内存
    void FixedSizePool::deallocate(void* p) // 释放小块内存
    {
        if (p == nullptr) return; //如果p是nullptr，表示没有需要释放的小块内存，直接返回  写法2： if (!p) return;
        Node* node = static_cast<Node*>(p); //将p转换为Node*类型，node指向需要释放的小块内存   目标类型* 变量名 = static_cast<目标类型*>(原始指针)
        node->next = free_list_; //将node的下一个节点指针指向当前链表的头节点，表示node现在是当前可用的小块内存的链表头节点，供后续的allocate操作使用
        free_list_ = node; //将链表的头节点指针更新为node，表示node现在是当前可用的小块内存的链表头节点，供后续的allocate操作使用
    }

    std::size_t FixedSizePool::adjust_block_size(std::size_t s) //调整小块内存的大小，使其满足对齐要求，通常是8字节对齐，以提高内存访问效率。
    {
        std::size_t min = sizeof(void*); //最小对齐要求，通常是指针大小
        std::size_t a = align_up(s < min ? min : s, alignof(void*)); //将s调整为满足对齐要求的大小，如果s小于最小对齐要求，则使用最小对齐要求，否则使用s本身，然后调用align_up函数进行对齐; 
        return a;
    }
    
    
    //每次向系统申请一页的内存，并把这一页内存切分成多个小块内存，并将这些小块内存加入到链表中，供后续的allocate和deallocate操作使用。
    void FixedSizePool::expand()
    {   //一整页内存的字节数
        std::size_t page_bytes = block_size_ * blocks_per_page_; //一整页内存的字节数=每个小块内存的大小*每页内存中小块内存的数量
        char* page = static_cast<char*>(::operator new(page_bytes)); //向系统申请一页内存，返回一个指向该内存的指针，并将其转换为char*类型，page指向新申请的一页内存的起始地址
        pages_.push_back(page); //将新申请的一页内存的地址加入到pages_列表中，供后续的析构函数释放使用

        //把一整页的内存分成blocks_per_page_个节点，每个节点的大小为block_size_，串成一个链表，链表的头节点指针为free_list_
        for (std::size_t i = 0; i < blocks_per_page_; i++)
        {
            char* addr = page + i * block_size_; //计算当前小块内存的地址，addr指向当前小块内存的起始地址，page是整页内存的起始地址，i是当前小块内存的索引，block_size_是每个小块内存的大小，通过page + i * block_size_计算出当前小块内存的地址
            Node* n = reinterpret_cast<Node*>(addr); //将当前小块内存的地址转换为Node*类型，n指向当前小块内存的起始地址，reinterpret_cast是C++中的一种类型转换操作符，用于在不同类型之间进行转换，这里将char*类型的addr转换为Node*类型，以便将其作为链表节点使用
            n->next = free_list_; //将n的下一个节点指针指向当前链表的头节点，表示n现在是当前可用的小块内存的链表头节点，供后续的allocate操作使用
            free_list_ = n; //将链表的头节点指针更新为n，表示n现在是当前可用的小块内存的链表头节点，供后续的allocate操作使用
        }
    }


//如何将小块内存给调用者使用,调用者如何去拿到这一块内存：               struct Node { Node* next; };
//维护一个链表，当调用者需要这一块小内存时，将链表的头节点给他用，       Node* free_list_;
//如果此时我又申请了一页内存（假设100个小块）进来，
//将这100个小块内存以头插法插入到链表的头部（增加或回收都采用头插法）