#include "threadsafe_stack.h"
#include "join_threads.h"
#include <list>
#include <future>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <functional>
#include <iostream>

template <typename Iterator, typename T>
struct accumulate_block {
	T operator()(Iterator first, Iterator last) {
		return std::accumulate(first, last, T());
	}
};

template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
	unsigned long const length = std::distance(first, last);
	if (!length)
		return init;

	unsigned long const min_per_thread = 25;
	unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;
	unsigned long const hardware_threads = std::thread::hardware_concurrency();
	unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads;

	std::vector<std::future<T>> futures(num_threads - 1);
	std::vector<std::thread> threads(num_threads - 1);
	// 1 当创建了线程容器，就对新类型创建了一个实例，可让退出线程进行汇入
	join_threads joiner(threads);

	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); i++) {
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		std::packaged_task<T(Iterator, Iterator)> task(accumulate_block<Iterator, T>());
		futures[i] = task.get_future();
		threads[i] = std::thread(std::move(task), block_start, block_end);
		block_start = block_end;
	}
	T last_result = accumulate_block()(block_start, last);
	//std::for_each(threads.begin()), threads.end(), std::mem_fn(&std::thread::join));

	T result = init;
	for (unsigned long i = 0; i < (num_threads - 1); ++i) {
		result += futures[i].get();// 2 将会阻塞线程，直到结果准备就绪
	}
	result += last_result;
	return result;
}

int main()
{
	std::vector<int> l{ 35,3,4,44,66,22,11,222,333,55,1,0,9,6,35,3,4,44,66,22,11,222,333,55,1,0,9,6 };
	std::cout << parallel_accumulate(l.begin(), l.end(), 0) << std::endl;
	system("pause");
	return 0;
}