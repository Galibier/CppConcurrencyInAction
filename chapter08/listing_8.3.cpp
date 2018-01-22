#include <list>
#include <future>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>

template <typename Iterator,typename T>
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
	
	std::vector<std::future<T>> futures(num_threads-1);
	std::vector<std::thread> threads(num_threads - 1);

	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); i++) {
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		/*std::packaged_task<> 对一个函数或可调用对象，绑定一个期望。
		当 std::packaged_task<>对象被调用，它就会调用相关函数或可调用对象，
		将期望状态置为就绪，返回值也会被存储为相关数据。*/
		std::packaged_task<T(Iterator, Iterator)> task(accumulate_block<Iterator, T>());
		futures[i] = task.get_future();
		// 6 将需要处理的数据块的开始和结束信息传入，让新线程去执行这个任务
		threads[i] = std::thread(std::move(task), block_start, block_end);
		block_start = block_end;
	}
	T last_result = accumulate_block()(block_start, last);
	std::for_each(threads.begin()), threads.end(), std::mem_fn(&std::thread::join));

	T result = init;
	/*当任务执行时，future将会获取对应的结果，以及任何抛出的异常。
	如果相关任务抛出一个异常，那么异常就会被future捕捉到，
	并且使用get()的时候获取数据时，这个异常会再次抛出。*/
	for (unsigned long i = 0; i < (num_threads - 1); ++i) {
		result += futures[i].get();
	}
	result += last_result;
	return result;
}
