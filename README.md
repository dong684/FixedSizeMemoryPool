# 高性能固定大小内存池 (Fixed-Size Memory Pool)

[![C++11](https://img.shields.io/badge/C++-11%2B-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20CentOS-lightgrey.svg)](https://www.centos.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)

## 1. 项目背景与动机

在实时渲染、游戏引擎粒子系统或高频交易逻辑中，程序需要频繁创建和销毁大量同尺寸的小对象。
原生 `std::new/delete` (基于 `glibc malloc`) 在此场景下存在以下缺陷：
- **系统调用开销**：频繁触发内核态切换，分配效率低。
- **内存碎片化**：导致物理内存不连续，降低实际利用率。
- **缓存局部性差**：离散的地址分布导致 CPU Cache Miss 率高。

**本项目通过预分配大块内存（Page）与侵入式链表（Intrusive Free List）技术，实现了时间复杂度为 $O(1)$ 的极致分配器。**

---

## 2. 核心架构设计

项目采用 **“Page-Block”** 两级内存布局，管理机制如下：

* **大页预分配 (Page Allocation)**：一次性向 OS 批发大容量内存，减少分配频率。
* **侵入式空闲链表 (Intrusive Free List)**：利用未分配 Block 的空间存储 `Node* next` 指针，实现 **Zero-overhead** 元数据管理。
* **内存对齐 (Alignment)**：基于位运算实现 8 字节（64位系统）自动对齐，确保访存效率。

---

## 3. 技术实现细节

### 3.1 内存对齐算法
使用高效位运算实现向上取整对齐：
`return (n + (align - 1)) & ~(align - 1);`
确保即使业务对象小于指针大小，也能安全存储链表节点。

### 3.2 资源生命周期 (RAII)
通过 `std::vector<void*> pages_` 追踪所有物理内存页，在内存池析构时统一调用 `::operator delete` 释放，从物理层面上杜绝内存泄漏。

### 3.3 业务无缝接入
利用 **Operator Overloading**（操作符重载）重载业务类的 `new` 与 `delete`，实现对原生分配逻辑的透明拦截。

---

## 4. 性能基准测试报告

**测试环境**：
- **OS**: CentOS 7.x (64-bit)
- **Compiler**: g++ (GCC) 开启 `-O3` 优化
- **Test Volume**: 10,000,000 (一千万) 次实例化/销毁

| 分配器类型 | 总耗时 (ms) | 性能提升 |
| :--- | :--- | :--- |
| **标准 glibc malloc** | ~545.01 ms | 基准 (1x) |
| **Fixed-Size Pool** | **~162.25 ms** | **🚀 3.35x** |

**结论**：测试证明，在千万级吞吐量下，本内存池比原生分配器快 **3.35 倍**。

---

## 5. 快速上手

### 编译与运行
```bash
# 包含源文件进行编译
g++ -O3 MemoryPool.cpp main.cpp -o memory_test

执行压力测试
./memory_test
