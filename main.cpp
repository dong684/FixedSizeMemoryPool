#include "MemoryPool.h" //包含了自定义的内存池类的头文件，提供了内存池的定义和接口。
#include <iostream> //提供了定义了输入输出流的头文件，如std::cout、std::cin等，用于进行输入输出操作。
#include <iomanip> //用于格式化输出
#include <chrono> // 专门用来计时的库
#include <vector> //提供了定义了std::vector容器的头文件，std::vector是一个动态数组，可以自动调整大小。

struct Particle
{
    float x, y, z; //位置坐标
    int life; //寿命

    static void* operator new(std::size_t n);//重载new运算符，使用内存池分配内存
    static void operator delete(void* p) noexcept; //重载delete运算符，使用内存池释放内存 noexcept表示该函数不会抛出异常，如果函数内部发生了异常，程序会调用std::terminate函数终止程序的执行。

    void update()
    {
        ++life; //更新粒子状态，增加寿命
    }
};

static FixedSizePool g_garticle_pool(sizeof(Particle), 4096); //全局粒子内存池，块大小为Particle结构体的大小，每页包含4096个小块内存

void* Particle::operator new(std::size_t n)//重载new运算符，使用内存池分配内存
{
    return g_garticle_pool.allocate();//从全局粒子内存池中分配内存，返回一个指向分配内存的指针
}

void Particle::operator delete(void* p) noexcept//重载delete运算符，使用内存池释放内存
{
    g_garticle_pool.deallocate(p); //将p指向的内存归还给全局粒子内存池
}


int main() 
{
    const int count = 10000000;


    //================== 第一部分：内存池测试 ==================
    auto start_pool = std::chrono::high_resolution_clock::now(); //记录内存池测试开始的时间点

    std::vector<Particle*> pool_vec;//使用std::vector容器来存储分配的粒子对象指针，方便后续的管理和释放
    
    pool_vec.reserve(count); //预先分配vec容器的内存，以避免在循环中频繁扩容，提升性能

    for (int i = 0; i < count; i++) 
    {
        Particle* p = new Particle{0,0,0,0}; //使用new运算符创建一个Particle对象，调用重载的new运算符，从内存池中分配内存，并返回一个指向新创建对象的指针
        pool_vec.push_back(p); //将p存储在vec容器中，以便后续使用和管理这些粒子对象
    }
    
    for (auto* p : pool_vec) delete p; //使用delete运算符释放每个粒子对象的内存，调用Particle类中重载的delete运算符，将内存归还给内存池
    
    auto end_pool = std::chrono::high_resolution_clock::now(); //记录内存池测试结束的时间点
   
    std::chrono::duration<double, std::milli> diff_pool = end_pool - start_pool;//计算内存池测试的耗时，单位为毫秒，diff_pool.count()返回耗时的数值，可以用于后续的输出和对比分析
    
    
    // ================== 第二部分：标准 new/delete 测试 ==================
   
    auto start_std = std::chrono::high_resolution_clock::now(); //记录标准new/delete测试开始的时间点
    
    std::vector<Particle*> std_vec;//使用std::vector容器来存储分配的粒子对象指针，方便后续的管理和释放
    
    std_vec.reserve(count); //预先分配vec容器的内存，以避免在循环中频繁扩容，提升性能
    
    for (int i = 0; i < count; i++) 
    {
        std_vec.push_back(::new Particle{0,0,0,0});//使用全局的new运算符创建一个Particle对象，调用标准的内存分配机制分配内存，并返回一个指向新创建对象的指针
    }
    
    for (auto* p : std_vec) ::delete p;
    
    auto end_std = std::chrono::high_resolution_clock::now(); //记录标准new/delete测试结束的时间点
    
    std::chrono::duration<double, std::milli> diff_std = end_std - start_std;

    // ================== 输出对比结果 ==================
    // ... 在计时逻辑之后 ...

    std::cout << "\n===========================================" << std::endl;
    std::cout << "        内存池性能测试报告 (CentOS)        " << std::endl;
    std::cout << "===========================================" << std::endl;

    // 1. 基础配置信息
    std::cout << "【配置信息】" << std::endl;
    std::cout << "  - 内存块大小 (BlockSize):  " << g_garticle_pool.block_size() << " 字节" << std::endl;
    std::cout << "  - 每页块数量 (BlocksPerPage): " << g_garticle_pool.blocks_per_page() << std::endl;
    std::cout << "  - 测试对象总数:           " << count << " 个" << std::endl;

    std::cout << "-------------------------------------------" << std::endl;

    // 2. 耗时对比 (保留 4 位小数)
    std::cout << "【耗时统计】" << std::fixed << std::setprecision(4) << std::endl;
    std::cout << "  - 标准 new/delete 耗时:    " << diff_std.count() << " ms" << std::endl;
    std::cout << "  - 内存池 allocate/dealloc: " << diff_pool.count() << " ms" << std::endl;

    std::cout << "-------------------------------------------" << std::endl;

    // 3. 结论
    double speedup = diff_std.count() / diff_pool.count();
    std::cout << "【测试结论】" << std::endl;
    if (speedup > 1.0) {
        std::cout << "  🚀 内存池比标准分配快了 " << speedup << " 倍！" << std::endl;
    } else {
        std::cout << "  ⚠️ 当前环境下内存池优势不明显。" << std::endl;
    }
    std::cout << "===========================================\n" << std::endl;

    return 0;
}