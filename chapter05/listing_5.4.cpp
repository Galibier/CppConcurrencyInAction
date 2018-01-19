#include <atomic>
#include <thread>
#include <assert.h>
//序列一致是最简单、直观的序列，但是他也是最昂贵的内存序列，因为它需要对所有线程进行全局同步。在一个多处理系统上，这就需要处理期间进行大量并且费时的信息交换。

std::atomic<bool> x, y;
std::atomic<int> z;

void write_x() {
	x.store(true, std::memory_order_seq_cst);//1
}

void write_y() {
	y.store(true, std::memory_order_seq_cst);//2
}

void read_x_then_y() {
	while (!x.load(std::memory_order_seq_cst));
	if (y.load(std::memory_order_seq_cst))//3
		++z;
}

void read_y_then_x() {
	while (!y.load(std::memory_order_seq_cst));
	if (x.load(std::memory_order_seq_cst))//4
		++z;
}

int main() {
	x = false;
	y = false;
	z = 0;
	std::thread a(write_x);
	std::thread b(write_y);
	std::thread c(read_x_then_y);
	std::thread d(read_y_then_x);
	a.join();
	b.join();
	c.join();
	d.join();
	assert(z.load() != 0);//z=1 or 2, never 0.
}