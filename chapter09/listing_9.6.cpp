#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <future>

template<typename T>
class thread_safe_queue
{
private:
	std::unique_ptr<node> try_pop_head()
	{
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail())
		{
			return std::unique_ptr<node>();
		}
		return pop_head();
	}

	std::unique_ptr<node> try_pop_head(T& value)
	{
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail())
		{
			return std::unique_ptr<node>();
		}
		value = std::move(*head->data);
		return pop_head();
	}

public:
	std::shared_ptr<T> try_pop()
	{
		std::unique_ptr<node> const old_head = try_pop_head();
		return old_head ? old_head->data : std::shared_ptr<T>();
	}

	bool try_pop(T& value)
	{
		std::unique_ptr<node> const old_head = try_pop_head(value);
		return old_head;
	}

	void empty()
	{
		std::lock_guard<std::mutex> head_lock(head_mutex);
		return (head == get_tail());
	}
};
class join_threads {
	std::vector<std::thread>& threads;
public:
	explicit join_threads(std::vector<std::thread>& threads_) :threads(threads_) {}
	~join_threads() {
		for (unsigned long i = 0; i < threads.size(); ++i) {
			if (threads[i].joinable())
				threads[i].join();
		}
	}
};

class function_wrapper {
	struct impl_base {
		virtual void call() = 0;
		virtual ~impl_base() {}
	};
	std::unique_ptr<impl_base> impl;
	template <typename F>
	struct impl_type : impl_base {
		F f;
		impl_type(F&& f_) : f(std::move(f_)) {}
		void call() { f(); }
	};
public:
	template <typename F>
	function_wrapper(F&& f) :impl(new impl_type<F>(std::move(f))) {}

	void operator()() { impl->call(); }
	function_wrapper() = default;
	function_wrapper(function_wrapper&& other) :impl(std::move(other.impl)) {}
	function_wrapper& operator=(function_wrapper&& other) {
		impl = std::move(other.impl);
		return *this;
	}
	function_wrapper(const function_wrapper&) = delete;
	function_wrapper(function_wrapper&) = delete;
	function_wrapper& operator=(const function_wrapper&) = delete;
};

class thread_pool {
	std::atomic_bool done;
	//全局队列
	thread_safe_queue<function_wrapper> pool_work_queue;
	// 1 普通队列，只有一个线程访问
	typedef std::queue<function_wrapper> local_queue_type;
	// 2 不希望非线程池中的线程也拥有一个任务队列，up指向线程本地的工作队列
	static thread_local std::unique_ptr<local_queue_type> local_work_queue;
	std::vector<std::thread> threads;
	join_threads joiner;

	void worker_thread() {
		local_work_queue.reset(new local_queue_type);
		while (!done) {
			run_pending_task();
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
	std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f) {
		typedef std::result_of<FunctionType()>::type result_type;

		std::packaged_task<result_type()> task(f);
		std::future<result_type> res(task.get_future());
		if (local_work_queue) {// 4 检查当前线程是否具有一个工作队列
			local_work_queue->push(std::move(task));
		}
		else {
			pool_work_queue.push(std::move(task));
		}
		return res;
	}

	void run_pending_task() {
		function_wrapper task;
        // 6 检查当前线程是否具有一个工作队列，该队列是否有任务
		if (local_work_queue && !local_work_queue->empty()) {
			task = std::move(local_work_queue->front());
			local_work_queue->pop();
			task();
		}
		else if (pool_work_queue.try_pop(task)) {// 7 从全局队列获取任务
			task();
		}
		else {
			std::this_thread::yield();
		}
	}
};