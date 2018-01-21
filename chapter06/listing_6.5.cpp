#include <memory>

template <typename T>
class queue {
private:
	struct node {
		std::shared_ptr<T> data;//数据指针
		std::unique_ptr<node> next;
	};

	std::unique_ptr<node> head;
	node* tail;

public:
	queue() :head(new node), tail(head.get()) {}//虚拟节点的创立
	queue(const queue& other) = delete;
	queue& operator=(const queue& other) = delete;

	std::shared_ptr<T> try_pop() {
		if (head.get()==tail) {//没有元素
			return std::shared_ptr<T>();
		}
		std::shared_ptr<T> const res(head->data);//取出返回数据指针
		std::unique_ptr<node> const old_head = std::move(head);
		head = std::move(old_head->next);
		return res;
	}

	void push(T new_value) {
		std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));//生成数据指针
		std::unique_ptr<node> p(new node);//新虚拟节点

		tail->data = new_data;//虚拟节点变为新节点
		node* const new_tail = p.get();
		tail->next = std::move(p);
		tail = new_tail;
	}
};