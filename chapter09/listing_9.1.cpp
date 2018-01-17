#include <atomic>
#include <functional>
#include <thread>
#include <vector>

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

class thread_pool {
	std::atomic_bool done;
	thread_safe_queue<std::function<void()>> work_queue;
	std::vector<std::thread> threads;
	join_threads joiner;

	void worker_thread() {
		while (!done) {
			std::function<void()> task;
			if (work_queue.try_pop(task)) {
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
		work_queue.push(std::function<void()>(f));
	}
};