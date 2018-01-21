#include <atomic>
#include <memory>

template<typename T>
class lock_free_stack {
private:
	struct node {
		std::shared_ptr<T> data;
		node* next;
		node(T const& data_) : data(std::make_shared<T>(data_)) {}
	};
	std::atomic<node*> head;
	std::atomic<unsigned> threads_in_pop;
	std::atomic<node*> to_be_deleted;

	static void delete_nodes(node* nodes) {
		while (nodes) {
			node* next = nodes->next;
			delete nodes;
			nodes = next;
		}
	}

	void try_reclaim(node* old_head) {
		if (threads_in_pop == 1) {
			node* nodes_to_delete = to_be_deleted.exchange(nullptr);
			if (!--threads_in_pop) {
				delete_nodes(nodes_to_delete);
			}
			else if (nodes_to_delete) {
				chain_pending_nodes(nodes_to_delete);
			}
			delete old_head;
		}
		else {
			chain_pending_node(old_head);
			--threads_in_pop;
		}
	}

	void chain_pending_nodes(node* nodes) {
		node* last = nodes;
		while (node* const next = last->next) {
			last = next;
		}
		chain_pending_nodes(nodes, last);
	}

	void chain_pending_nodes(node* first, node* last) {
		last->next = to_be_deleted;
		while (!to_be_deleted.compare_exchange_weak(last->next, first))
			;
	}

	void chain_pending_nodes(node* n) {
		chain_pending_nodes(n, n);
	}

public:
	void push(T const& data) {
		node* const new_node = new node(data);
		new_node->next = head.load();
		while (!head.compare_exchange_weak(new_node->next, new_node));
	}
	std::shared_ptr<T> pop() {
		std::atomic<void*>& hp = get_hazard_pointer_for_current_thread();//可以返回风险指针的引用
		node* old_head = head.load();//确保正确设置
		do {//循环保证node不会在读取旧head指针时，以及在设置风险指针的时被删除
			node* temp;
			do {// 1 直到将风险指针设为head指针
				temp = old_head;
				hp.store(old_head);
				old_head = head.load();
			} while (old_head != temp);
		} while (old_head && !head.compare_exchange_strong(old_head, old_head->next));
		hp.store(nullptr);// 2 当声明完成，清除风险指针
		std::shared_ptr<T> res;
		if (old_head) {
			res.swap(old_head->data);
			if (outstanding_hazard_pointers_for(old_head)) {// 3 在删除之前对风险指针引用的节点进行检查，是否被引用
				reclaim_later(old_head);// 4 有，存放链表中
			}
			else {
				delete old_head;// 5 没有，直接删除
			}
			delete_nodes_with_no_hazards();// 6 检查链表，是否有风险指针引用
		}
		return res;
	}
};