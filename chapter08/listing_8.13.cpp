#include <list>
#include <future>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <iostream>
#include "join_threads.h"

class barrier {
	std::atomic<unsigned> count;  //改为原子变量，多线程对其进行更新的时候，就不需要添加额外的同步
	std::atomic<unsigned> spaces;
	std::atomic<unsigned> generation;
public:
	explicit barrier(unsigned count_) : count(count_), spaces(count_), generation(0) {}
	void wait() {
		unsigned const my_generation = generation;
		if (!--spaces) {
			spaces = count.load();
			++generation;
		}
		else {
			while (generation == my_generation)
				std::this_thread::yield();
		}
	}
	void done_waiting() {//当一个线程完成其工作，并在等待的时候，才能对其进行调用它：
		--count;//1 递减，下次重置spaces将会变小
		if (!--spaces) {
			spaces = count.load();
			++generation;//3 一组当中最后一个线程需要对计数器进行重置，并且递增generation的值
		}
	}
};

template <typename Iterator>
void parallel_partial_sum(Iterator first, Iterator last) {
	typedef typename Iterator::value_type value_type;
	struct process_element {
		void operator()(Iterator first, Iterator last, std::vector<value_type>& buffer, unsigned i, barrier& b) {
			value_type& ith_element = *(first + i);
			bool updata_source = false;
			for (unsigned step = 0, stride = 1; stride <= i; ++step, stride *= 2) {
				value_type const source = (step % 2) ? buffer[i] : ith_element;// 2 每一步，都会从原始数据或缓存中获取第i个元素
				value_type& dest = (step % 2) ? ith_element : buffer[i];
				value_type const addend = (step % 2) ? buffer[i - stride] : *(first + i - stride);// 3 获取到的元素加到指定stride的元素中去
				dest = source + addend;
				updata_source = !(step % 2);
				b.wait();
			}
			if (updata_source) {
				ith_element = buffer[i];
			}
			b.done_waiting();
		}
	};
	unsigned long const length = std::distance(first, last);
	if (length <= 1)
		return;

	std::vector<value_type> buffer(length);
	barrier b(length);
	std::vector<std::thread> threads(length - 1);//线程的数量是根据列表中的数据量来定的，并非硬件支持数（api获取）
	join_threads joiner(threads);

	Iterator block_start = first;
	for (unsigned long i = 0; i < (length - 1); ++i) {
		threads[i] = std::thread(process_element(), first, last, std::ref(buffer), i, std::ref(b));
	}
	process_element()(first, last, buffer, length - 1, b);
}

int main() {
	std::vector<int> v{ 1,2,3,4,5 };
	parallel_partial_sum(v.begin(), v.end());
	std::for_each(v.begin(), v.end(), [](const int &i) {std::cout << i << std::endl; });//lambda参数是元素类型
	return 0;
}