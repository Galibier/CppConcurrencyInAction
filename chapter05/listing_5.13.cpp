#include <atomic>
#include <thread>
#include <assert.h>

bool x = false;
std::atomic<bool> y;
std::atomic<int> z;

void write_x_then_y() {
	x = true;//// 1 在栅栏前存储x
	std::_Atomic_thread_fence(std::memory_order_release);
	y.store(true, std::memory_order_relaxed);// 2 在栅栏后存储y
}

void read_y_then_x() {
	while (!y.load(std::memory_order_relaxed));// 3 在#2写入前，持续等待
	std::_Atomic_thread_fence(std::memory_order_acquire);
	if (x)// 4 这里读取到的值，是#1中写入
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
	assert(z.load() != 0);// 5 断言将不会触发
}