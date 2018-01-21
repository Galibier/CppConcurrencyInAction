#include <atomic>

//无锁数据结构：线程可以并发的访问。具有“比较/交换”操作的数据结构，通常在“比较/交换”实现中都有一个循环。
//使用“比较/交换”操作的原因：当有其他线程同时对指定数据的修改时，代码将尝试恢复数据。
class spinlock_mutex {
	std::atomic_flag flag;
public:
	spinlock_mutex() :flag(ATOMIC_FLAG_INIT) {}
	void lock() {
		while (flag.test_and_set(std::memory_order_acquire));
	}
	void unlock() {
		flag.clear(std::memory_order_release);
	}
};