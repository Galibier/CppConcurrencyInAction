#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template<typename T>
class threadsafe_queue {
private:
	mutable std::mutex mut;
	std::queue<std::shared_ptr<T>> data_queue;
	std::condition_variable data_cond;
public:
	threadsafe_queue() {}

	void wait_and_pop(T& value) {
		std::unique_lock<std::mutex> lk(mut);

		data_cond.wait(lk, [this] {return !data_queue.empty(); });/***********************/

		value = std::move(*data_queue.front());
		data_queue.pop();
	}

	bool try_pop(T& value) {
		std::lock_guard<std::mutex> lk(mut);

		if (data_queue.empty())/**************************/
			return false;

		value = std::move(*data_queue.front());
		data_queue.pop();
	}

	std::shared_ptr<T> wait_and_pop() {
		std::unique_lock<std::mutex> lk(mut);

		data_cond.wait(lk, [this] {return !data_queue.empty(); });/************************/

		std::shared_ptr<T> res = data_queue.front();
		data_queue.pop();
		return res;
	}

	std::shared_ptr<T> try_pop() {
		std::lock_guard<std::mutex> lk(mut);

		if (data_queue.empty())/**************************/
			return std::shared_ptr<T>();

		std::shared_ptr<T> res = data_queue.front();
		data_queue.pop();
		return res;
	}

	void push(T new_value) {
		//push中实例分配完毕，唤醒线程，不会发生内存分配失败（不用构造新的std::shared_ptr<>实例，分配内存），也就不会锁在线程（继续等待push，其他线程沉睡）中。
		//同时，内存分配的方式提升了队列工作效率，在分配内存的时候，可以让其他线程对队列进行操作。
		std::shared_ptr<T> data(std::make_shared<T>(std::move(new_value)));
		std::lock_guard<std::mutex> lk(mut);
		data_queue.push(data);
		data_cond.notify_one();
	}
};

int main() {}