#include <atomic>
#include <memory>

template<typename T>
class lock_free_queue {
private:
	struct node {
		std::shared_ptr<T> data;
		node* next;
		node() :next(nullptr) {}
	};
	std::atomic<node*> head;
	std::atomic<node*> tail;
	node* pop_head() {
		node* const old_head = head.load();
		if (old_head == tail.load()) {
			return nullptr;
		}
		head.store(old_head->next);
		return old_head;
	}

public:
	lock_free_queue() :head(new node), tail(head.load()) {}
	lock_free_queue(const lock_free_queue& other) = delete;
	lock_free_queue& operator=(const lock_free_queue& other) = delete;
	~lock_free_queue() {
		while (node* const old_head = head.load()) {
			head.store(old_head->next);
			delete old_head;
		}
	}
	std::shared_ptr<T> pop() {
		node* old_head = pop_head();
		if (!old_head) {
			return std::shared_ptr<T>();
		}
		std::shared_ptr<T> const res(old_head->data);
		delete old_head;
		return res;
	}
	void push(T new_value) {
		std::unique_ptr<T> new_data(new T(new_value));
		counted_node_ptr new_next;
		new_next.ptr = new node;
		new_next.external_count = 1;
		for (;;) {
			node* const old_tail = tail.load();
			T* old_data = nullptr;
			if (old_tail->data.compare_exchange_strong(old_data, new_data.get())) {
				old_tail->next = new_next;
				tail.store(new_next.ptr);
				new_data.release();
				break;
			}
		}
	}
};