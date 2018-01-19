#include <utility>
#include <atomic>
#include <vector>
#include <thread>

std::vector<int> queue_data;
std::atomic<int> count;

void populate_queue() {
	unsigned const number_of_items = 20;
	queue_data.clear();
	for (unsigned i = 0; i < number_of_items; i++) {
		queue_data.push_back(i);
	}
	count.store(number_of_items, std::memory_order_release);// 1 初始化存储
}

void consume_queue_items() {
	while (true) {
		int item_index;
		if ((item_index = count.fetch_sub(1, std::memory_order_acquire)) <= 0) {// 2 一个“读-改-写”操作
			wait_for_more_items();// 3 等待更多元素
			continue;
		}
		prcocess(queue_data[item_index - 1]);// 4 安全读取queue_data
	}
}

int main(){
	std::thread a(populate_queue);
	std::thread b(consume_queue_items);
	std::thread c(consume_queue_items);
	a.join();
	b.join();
	c.join();
}