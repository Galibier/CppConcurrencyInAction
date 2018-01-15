#include <atomic>
#include <memory>

template<typename T>
class lock_free_stack{
private:
	struct node {
		std::shared_ptr<T> data;
		node* next;
		node(T const& data_) : data(std::make_shared<T>(data_)) {}
	};
	std::atomic<node*> head;
	std::atomic<unsigned> threads_in_pop;
	void try_reclaim(node* old_head);
public:
	void push(T const& data) {
		node* const new_node = new node(data);
		new_node->next = head.load();
		while (!head.compare_exchange_weak(new_node->next, new_node));
	}
	std::shared_ptr<T> pop() {
		++threads_in_pop;
		node* old_head = head.load();
		while (old_head && !head.compare_exchange_weak(old_head, old_head->next))
			;
		std::shared_ptr<T> res;
		if (old_head) {
			res.swap(old_head->data);
		}
		try_reclaim(old_head);
		return res;
	}
};