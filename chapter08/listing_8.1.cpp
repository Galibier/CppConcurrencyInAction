#include "threadsafe_stack.h"
#include <list>
#include <future>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <iostream>

template <typename T>
struct sorter {
	struct chunk_to_sort {
		std::list<T> data;
		std::promise<std::list<T>> promise;
	};

	threadsafe_stack<chunk_to_sort> chunks;
	std::vector<std::thread> threads;// 3 对线程进行设置
	unsigned const max_thread_count;
	std::atomic<bool> end_of_data;

	sorter() :max_thread_count(std::thread::hardware_concurrency() - 1), end_of_data(false) {}
	~sorter() {
		end_of_data = true;
		for (unsigned i = 0; i < threads.size(); ++i) {
			threads[i].join();
		}
	}

	void try_sort_chunk() {
		std::shared_ptr<chunk_to_sort> chunk = chunks.pop();
		if (chunk) {
			sort_chunk(chunk);
		}
	}

	std::list<T> do_sort(std::list<T>& chunk_data) {
		if (chunk_data.empty()) {
			return chunk_data;
		}
		std::list<T> result;
		result.splice(result.begin(), chunk_data, chunk_data.begin());
		T const& partition_val = *result.begin();
		typename std::list<T>::iterator divide_point =//对数据进行划分
			std::partition(chunk_data.begin(), chunk_data.end(), [&](T const& val) {return val < partition_val; });
		chunk_to_sort new_lower_chunk;
		//此函数操作是移动元素
		new_lower_chunk.data.splice(new_lower_chunk.data.end(), chunk_data, chunk_data.begin(), divide_point);

		std::future<std::list<T>> new_lower = new_lower_chunk.promise.get_future();
		chunks.push(std::move(new_lower_chunk));// 11 将这些数据块的指针推到栈上
		if (threads.size() < max_thread_count) {// 12 有备用处理器的时候，产生新线程
			threads.push_back(std::thread(&sorter<T>::sort_thread, this));
		}
		//只剩高值部分，排序由当前线程完成
		std::list<T> new_higher(do_sort(chunk_data));
		result.splice(result.end(), new_higher);
		while (new_lower.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
			try_sort_chunk();
		}
		result.splice(result.begin(), new_lower.get());
		return result;
	}

	void sort_chunk(std::shared_ptr<chunk_to_sort>const &chunk) {
		// 15 结果存在promise中，让线程对已经存在于栈上的数据块进行提取
		chunk->promise.set_value(do_sort(chunk->data));
	}

	void sort_thread() {
		while (!end_of_data) {
			try_sort_chunk();
			// 17 新生成的线程还在尝试从栈上获取需要排序的数据块
			std::this_thread::yield();
			/*18 在循环检查中，也要给其他线程机会，可以从栈上取下数据块进行更多的操作。
			*Provides a hint to the implementation to reschedule the execution of threads, allowing other threads to run.
			*/
		}
	}
};

template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input) {
	if (input.empty()) {
		return input;
	}
	sorter<T> s;
	return s.do_sort(input);
}

int main()
{
	std::list<int> l{ 35,3,4,44,66,22,11,222,333,55,1,0,9,6,35,3,4,44,66,22,11,222,333,55,1,0,9,6 };
	auto r = parallel_quick_sort(l);//18ms
	for (auto &im : r) {
		std::cout << im << std::endl;
	}
	system("pause");
	return 0;
}