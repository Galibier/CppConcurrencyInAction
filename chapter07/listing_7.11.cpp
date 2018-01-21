#include <atomic>
#include <memory>

template <typename T>
class lock_free_stack {
private:
	struct node;
	struct counted_node_ptr {
		int external_count;
		node* ptr;
	};
	struct node {
		std::shared_ptr<T> data;
		std::atomic<int> internal_count;
		counted_node_ptr next;
		node(T const& data_) :data(std::make_shared<T>(data_)), internal_count(0) {}
	};
	std::atomic<counted_node_ptr> head;

	void increase_head_count(counted_node_ptr& old_counter) {
		counted_node_ptr new_counter;
		do {
			new_counter = old_counter;
			++new_counter.external_count;
		} while (!head.compare_exchange_strong(old_counter, new_counter));
		// 1 保证指针不会在同一时间内被其他线程修改
		old_counter.external_count = new_counter.external_count;
	}
public:
	~lock_free_stack() {
		while (pop());
	}
	void push(T const& data) {
		counted_node_ptr new_node;
		new_node.ptr = new node(data);
		new_node.external_count = 1;
		new_node.ptr->next = head.load();
		while (!head.compare_exchange_weak(new_node.ptr->next, new_node))
			;
	}
	
	std::shared_ptr<T> pop() {
		counted_node_ptr old_head = head.load();
		for (;;) {
			increase_head_count(old_head);
			node* const ptr = old_head.ptr;// 2 当计数增加，就能安全的解引用
			if (!ptr) {
				return std::shared_ptr<T>();
			}
			if (head.compare_exchange_strong(old_head, ptr->next)) {//删除节点
				std::shared_ptr<T> res;
				res.swap(ptr->data);
				int const count_increase = old_head.external_count - 2;
				/* 5 相加的值要比外部引用计数少2。
                *当节点已经从链表中删除，就要减少一次计数
                *（这是基本的一次减去，相当于减在内部计数），
                *并且这个线程无法再次访问指定节点，所以还要再减一
                */
				if (ptr->internal_count.fetch_add(count_increase) == -count_increase) {
					delete ptr;
				}
				return res;
			}
			else if (ptr->internal_count.fetch_sub(1) == 1) {//返回相减前的值，如果当前线程是最后一个持有引用线程
				delete ptr;
			}
		}
	}
};