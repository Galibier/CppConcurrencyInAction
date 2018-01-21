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
	std::atomic<node*> to_be_deleted;

	static void delete_nodes(node* nodes) {
		while (nodes) {
			node* next = nodes->next;
			delete nodes;
			nodes = next;
		}
	}

	void try_reclaim(node* old_head) {//节点调用即删除（于原链表）
		if (threads_in_pop == 1) {//当前线程正对pop访问
			node* nodes_to_delete = to_be_deleted.exchange(nullptr);
            // 2 声明“可删除”列表，该函数返回：The contained value before the call.
            //下面进行检查，决定删除等待链表，还是还原to_be_delted
			if (!--threads_in_pop) {//是否只有一个线程调用pop（）
				delete_nodes(nodes_to_delete);//迭代删除等待链表
			}
			else if (nodes_to_delete) {//存在
				chain_pending_nodes(nodes_to_delete);// 6 nodes还原to_be_deleted=nodes_to_delete
			}
			delete old_head;
		}
		else {
			chain_pending_node(old_head);// 8 向等待列表中继续添加节点node
										 //此时to_be_deleted为old_head
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
		while (!to_be_deleted.compare_exchange_weak(last->next, first))// 11 用循环来保证last->next的正确性，存储第一个节点
			;
	}

	void chain_pending_nodes(node* n) {
		chain_pending_nodes(n, n);/*添加单个节点是一种特殊情况，因为这需要将这个节点作为第一个节点，同时也是最后一个节点进行添加。*/
	}

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