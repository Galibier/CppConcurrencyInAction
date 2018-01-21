#include <atomic>
#include <memory>

template<typename T>
class lock_free_queue {
private:
	struct node;
	struct counted_node_ptr {
		int external_node_ptr;
		node* ptr;
	};
	std::atomic<counted_node_ptr> head;
	std::atomic<counted_node_ptr> tail;
	/*重要的是，为的就是避免条件竞争，
    *将结构体作为一个单独的实体来更新。
    *让结构体的大小保持在一个机器字内，对其的操作就如同原子操作一样，
    *还可以在多个平台上使用。
    */
	struct node_counter {
		unsigned internal_count : 30;
		unsigned external_counters : 2;//表示多少指针指向
	};
	struct node {
		std::atomic<T*> data;
		std::atomic<node_counter> count;//原子类型
		counted_node_ptr next;
		node() {
			node_counter new_count;
			new_count.internal_count = 0;
			new_count.external_counters = 2;//node初始化
			count.store(new_count);//存入
			next.ptr = nulptr;
			next.external_count = 0;
		}
	};

public:
	void push(T new_value) {
		std::unique_ptr<T> new_data(new T(new_value));
		counted_node_ptr new_next;
		new_next.ptr = new node;
		new_next.external_count = 1;
		counted_node_ptr old_tail = tail.load();
		for (;;) {
			increase_external_count(tail, old_tail);//增加计数器计数
			T* old_data = nullptr;
			if (old_tail.ptr->data.compare_exchange_strong(old_data, new_data.get())) {
				old_tail.ptr->next = new_next;
				old_tail = tail.exchange(new_next);
				free_external_counter(old_tail);// 7 尾部旧值，计数器更新（external_counters等）
				new_data.release();//new_data再无所有权
				break;
			}
			old_tail.ptr->release_ref();
		}
	}
};