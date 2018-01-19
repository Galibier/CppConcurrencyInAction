#include <atomic>
#include <thread>
#include <cassert>
//栅栏操作会对内存序列进行约束，使其无法对任何数据进行修改，典型的做法是与使用memory_order_relaxed约束序的原子操作一起使用。
//栅栏属于全局操作，执行栅栏操作可以影响到在线程中的其他原子操作。因为这类操作就像画了一条任何代码都无法跨越的线一样，所以栅栏操作通常也被称为“内存栅栏”(memory barriers)。

std::atomic<bool> x, y;
std::atomic<int> z;

void write_x_then_y() {
	x.store(true, std::memory_order_relaxed);//1
	std::atomic_thread_fence(std::memory_order_release);//2
	y.store(true, std::memory_order_relaxed);//3
}

void read_y_then_x() {
	while (!y.load(std::memory_order_relaxed));//4
	std::atomic_thread_fence(std::memory_order_acquire);//5
	if (x.load(std::memory_order_relaxed))//6
		++z;
}

int main() {
	x = false;
	y = false;
	z = 0;
	std::thread a(write_x_then_y);
	std::thread b(read_y_then_x);
	a.join();
	b.join();
	assert(z.load() != 0);// no
}