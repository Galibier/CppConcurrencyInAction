#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_
#include <atomic>
#include <future>
#include "join_threads.h"
#include "threadsafe_queue.h"
#include "function_wrapper.h"
#include "work_stealing_queue.h"

class thread_pool {
	typedef function_wrapper task_type;
	std::atomic_bool done;
	// 使用function_wrapper，而非使用std::function（需要存储可复制构造的函数对象）
	threadsafe_queue<task_type> pool_work_queue;
	std::vector<std::unique_ptr<work_stealing_queue>> queues;
	std::vector<std::thread> threads;
	join_threads joiner;
	// 2 每个线程都有work_stealing_queue，后入先出
	static thread_local work_stealing_queue* local_work_queue;
	static thread_local unsigned my_index;

	void worker_thread(unsigned my_index_) {
		my_index = my_index_;
		local_work_queue = queues[my_index].get();
		while (!done) {
			run_pending_task();
		}
	}
	bool pop_task_from_local_queue(task_type& task) {
		return local_work_queue && local_work_queue->try_pop(task);
	}
	bool pop_task_from_pool_queue(task_type& task) {
		return pool_work_queue.try_pop(task);
	}
	bool pop_task_from_other_thread_queue(task_type& task) {
		/*为了避免每个线程都尝试从列表中的第一个线程上窃取任务，
		*每一个线程都会从下一个线程开始遍历，
		*通过自身的线程序号来确定开始遍历的线程序号。
		*/
		for (unsigned i = 0; i < queues.size(); ++i) {
			unsigned const index = (my_index + i + 1) % queues.size(); // 5
			if (queues[index]->try_steal(task)) {
				return true;
			}
		}
		return false;
	}

public:
	thread_pool() :done(false), joiner(threads) {
		unsigned const thread_count = std::thread::hardware_concurrency();
		try {
			for (unsigned i = 0; i < thread_count; ++i) {
				queues.push_back(std::unique_ptr<work_stealing_queue>(new work_stealing_queue));
				threads.push_back(std::thread(&thread_pool::worker_thread, this, i));
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

	// 1 返回一个 std::future<> 保存任务的返回值，并且允许调用者等待任务完全结束。
	template<typename FunctionType>
	std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f) {
		typedef typename std::result_of<FunctionType()>::type result_type;
		std::packaged_task<result_type()> task(f);
		std::future<result_type> res(task.get_future());
		if (local_work_queue) {
			local_work_queue->push(std::move(task));
		}
		else {
			pool_work_queue.push(std::move(task));
		}
		return res;
	}
	void run_pending_task() {
		function_wrapper task;
		if (pop_task_from_local_queue(task) || // 7 从线程的任务队列中取出一个任务来执行
			pop_task_from_pool_queue(task) || // 8 或从线程池队列中获取一个任务
			pop_task_from_other_thread_queue(task)) {// 9 亦或从其他线程的队列中获取一个任务（会遍历池中所有线程的任务队列，然后尝试窃取任务）
			task();
		}
		else {
			std::this_thread::yield();
		}
	}
};

//非stealing版本
//class thread_pool {
//	std::atomic_bool done;
//	// 使用function_wrapper，而非使用std::function（需要存储可复制构造的函数对象）
//	threadsafe_queue<function_wrapper> work_queue;
//	std::vector<std::thread> threads;//一组工作线程
//	join_threads joiner;
//
//	void worker_thread() {
//		while (!done) {
//			function_wrapper task;
//			if (work_queue.try_pop(task)) {
//				task();
//			}
//			else {
//				std::this_thread::yield();
//			}
//		}
//	}
//
//public:
//	thread_pool() :done(false), joiner(threads) {
//		unsigned const thread_count = std::thread::hardware_concurrency();
//		try {
//			for (unsigned i = 0; i < thread_count; ++i) {
//				threads.push_back(std::thread(&thread_pool::worker_thread, this));
//			}
//		}
//		catch (...) {
//			done = true;
//			throw;
//		}
//	}
//
//	~thread_pool() {
//		done = true;
//	}
//
//	// 1 返回一个 std::future<> 保存任务的返回值，并且允许调用者等待任务完全结束。
//	template<typename FunctionType>
//	std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f) {
//		typedef typename std::result_of<FunctionType()>::type result_type;
//		std::packaged_task<result_type()> task(std::move(f));
//		std::future<result_type> res(task.get_future());
//		work_queue.push(std::move(task));
//		return res;
//	}
//	void run_pending_task() {
//		function_wrapper task;
//		if (work_queue.try_pop(task)) {
//			task();
//		}
//		else {
//			std::this_thread::yield();
//		}
//	}
//};

#endif // !_THREAD_POOL_H_
#pragma once
