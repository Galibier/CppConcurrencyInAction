#include <atomic>
#include <thread>
#include <assert.h>
//非排序一致内存模型：线程没必要去保证一致性。在没有明确的顺序限制下，唯一的要求就是，所有线程都要统一对每一个独立变量的修改顺序。
//对不同变量的操作可以体现在不同线程的不同序列上，提供的值要与任意附加顺序限制保持一致。踏出排序一致世界后，最好的示范就是使用memory_order_relaxed对所有操作进行约束。
std::atomic<bool> x, y;
std::atomic<int> z;

void write_x_then_y() {
	x.store(true, std::memory_order_relaxed);//1
	y.store(true, std::memory_order_relaxed);//2
}

void read_y_then_x() {
	while (!y.load(std::memory_order_relaxed));//3
	if (x.load(std::memory_order_relaxed))//4
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
	assert(z.load() != 0);//5.有可能触发
}