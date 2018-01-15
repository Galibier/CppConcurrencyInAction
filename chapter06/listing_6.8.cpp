#include <mutex>
#include <memory>

template<typename T>
class threadsafe_queue
{
private:
	struct node {
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;
	};

	std::mutex head_mutex;
	std::unique_ptr<node> head;
	std::mutex tail_mutex;
	node* tail;
	std::condition_variable data_cond;

public:
	threadsafe_queue() :head(new node), tail(head.get()) {}
	threadsafe_queue(const threadsafe_queue& other) = delete;
	threadsafe_queue& operator(threadsafe_queue& other) = delete;

	std::shared_ptr<T> try_pop() {
		std::unique_ptr<node> old_head = pop_head();
		return old_head ? old_head->data : std::shared_ptr<T>();
	}

	std::shared_ptr<T> try_pop();
	bool try_pop(T& value);
	std::shared_ptr<T> wait_and_pop();
	void wait_and_pop(T& value);
	void push(T new_value);
	void empty();
};

template<typename T>
void threadsafe_queue<T>::push(T new_value){
	std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
	std::unique_ptr<node> p(new node);
	{
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		tail->data = new_data;
		node* const new_tail = p.get();
		tail->next = std::move(p);
		tail = new_tail;
	}
	data_cond.notify_one();
}