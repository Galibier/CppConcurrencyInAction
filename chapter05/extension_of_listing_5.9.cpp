#include <atomic>
#include <cassert>
#include <thread>
std::atomic<int> data[5];
std::atomic<int> sync(0);
void thread_1() {
	data[0].store(42, std::memory_order_relaxed);
	data[1].store(97, std::memory_order_relaxed);
	data[2].store(17, std::memory_order_relaxed);
	data[3].store(-141, std::memory_order_relaxed);
	data[4].store(2003, std::memory_order_relaxed);
	sync.store(1, std::memory_order_release);
}

void thread_2() {
	int expected = 1;
	while (!sync.compare_exchange_strong(expected, 2,//保证thread_1对变量只进行一次更新
		std::memory_order_acq_rel))//同时进行获取和释放的语义
		expected = 1;
	/*当expected与sync相等时候，sync=2，返回true，结束程序；
	*当expected与sync不等的时候，expected=sync，返回false，
	进入循环，expected=1；
	*/
}

void thread_3() {
	while (sync.load(std::memory_order_acquire) < 2);
	assert(data[0].load(std::memory_order_relaxed) == 42);
	assert(data[1].load(std::memory_order_relaxed) == 97);
	assert(data[2].load(std::memory_order_relaxed) == 17);
	assert(data[3].load(std::memory_order_relaxed) == -141);
	assert(data[4].load(std::memory_order_relaxed) == 2003);
}

int main() {
	std::thread t1{ thread_1 }, t2{ thread_2 }, t3{ thread_3 };
	t1.join();
	t2.join();
	t3.join();
	return 0;
}