#include <mutex>
#include <condition_variable>
#include <queue>

template <typename T>
class threadsafe_queue {
private:
	std::mutex mut;
	std::queue<T> data_queue;
	std::condition_variable data_cond;
public:
	/*threadsafe_queue();
	threadsafe_queue(const threadsafe_queue&);
	threadsafe_queue& operator=(const threadsafe_queue&) = delete;*/

	void push(T new_value) {
		std::lock_guard<std::mutex> lk(mut);
		data_queue.push(new_value);
		data_cond.notify_one();
	}

	bool try_pop(T& value);
	std::shared_ptr<T> try_pop();

	void wait_and_pop(T& value) {
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		value = data_queue.front();
		data_queue.pop();
	}
	std::shared_ptr<T> wait_and_pop();

	bool empty() const;
};

struct data_chunk {};

data_chunk prepare_data();
bool more_data_to_prepare();
void process(data_chunk);
bool is_last_chunk(data_chunk);

threadsafe_queue<data_chunk> data_queue;

void data_preparation_thread() {
	while (more_data_to_prepare()) {
		const data_chunk data = prepare_data();
		data_queue.push(data);
	}
}

void data_processing_thread() {
	while (true) {
		data_chunk data;
		data_queue.wait_and_pop(data);
		process(data);
		if (is_last_chunk(data))
			break;
	}
}

int main() {}