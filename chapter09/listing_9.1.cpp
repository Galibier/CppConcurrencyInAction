#include <atomic>
#include <functional>
#include <thread>
#include <vector>

class thread_pool {
	//注意成员声明顺序，使得以正确的顺序销毁
	std::atomic_bool done;
	thread_safe_queue<std::function<void()>> work_queue;// 1 线程安全队列管理任务队列
	std::vector<std::thread> threads;// 2 一组工作线程
	join_threads joiner;// 3 汇聚线程

	void worker_thread() {
		while (!done) {
			std::function<void()> task;
			if (work_queue.try_pop(task)) { // 5 从任务队列上获取队列
				task();
			}
			else {
				std::this_thread::yield();
			}
		}
	}
public:
	thread_pool() :done(false), joiner(threads) {
		unsigned const thread_count = std::thread::hardware_concurrency();
		try {
			for (unsigned i = 0; i < thread_count; ++i) {
				threads.push_back(std::thread(&thread_pool::worker_thread, this));
			}
		}
		catch (...) {
			done = true;
			throw;
		}
	}

	~thread_pool() {
		done = true;
	}

	template<typename FunctionType>
	void submit(FunctionType f) {
		work_queue.push(std::function<void()>(f));// 12 包装函数，压入队列
	}
};