#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

class spinlock_mutex {
	std::atomic_flag flag;//= ATOMIC_FLAG_INIT;
public:
	//初始化标志是“清除”，并且互斥量处于解锁状态。
	//std::atomic_flag 类型的对象必须被ATOMIC_FLAG_INIT初始化，总是初始化为清除。
	spinlock_mutex() : flag{ ATOMIC_FLAG_INIT } {}//此处使用新标准统一的初始化方式，否则就是调用复制构造函数（已删除）
	void lock() {
		//循环运行test_and_set()，直到旧值为false，当前线程设置为true
		while (flag.test_and_set(std::memory_order_acquire));
	}
	void unlock() {
		flag.clear(std::memory_order_release);
	}
};

struct test {
	spinlock_mutex m;
	int i;
};

void fun(test &t) {
	std::lock_guard<spinlock_mutex> lk(t.m);
	++t.i;
	std::cout << "thread: fun # " << t.i << std::endl;
}

int main() {
	test t;
	std::lock_guard<spinlock_mutex> lk(t.m);
	t.i = 0;
	lk.~lock_guard();
	std::vector<std::thread> vtds;
	for (int i = 0; i < 10; ++i) {
		vtds.push_back(std::thread{ fun, std::ref(t) });
	}
	for (auto &th : vtds) {
		th.join();
	}
	system("pause");
	return 0;
}