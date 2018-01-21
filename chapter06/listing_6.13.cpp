#include <memory>
#include <mutex>

template <typename T>
class threadsafe_list {
	struct node {
		std::mutex m;
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;

		node() :next() {}
		node(T const& value) :data(std::make_shared<T>(value)) {}
	};
	node head;

public:
	threadsafe_list() {}
	threadsafe_list() {
		remove_if([](T const&) {return true; });
	}

	threadsafe_list(threadsafe_list const& other) = delete;
	threadsafe_list& operator=(threadsafe_list const& other) = delete;

	void push_front(T const& value) {
		std::unique_ptr<node> new_node(new node(value));//指向新节点的指针
		std::lock_guard<std::mutex> lk(head.m);
		new_node->next = std::move(head.next);//修改指针，unique只能move
		head.next = std::move(new_node);
	}

	//将风险都转移到传入的谓词（函数），交由用户决定处理
	template<typename Function>
	void for_each(Function f) {// 7，模板函数出现新的类
		node* current = &head;
		std::unique_lock<std::mutex> lk(head.m);
		while (node* const next = current->next.get()) {//遍历
			std::unique_lock<std::mutex> next_lk(next->m);//逐个上锁
			lk.unlock();//前驱结点解锁
			f(*next->data);//对数据处理
			current = next;
			lk = std::move(next_lk);//将锁移至前驱
		}
	}

	template<typename Predicate>
	std::shared_ptr<T> find_first_if(Predicate p) {
		node* current = &head;
		std::unique_lock<std::mutex> lk(head.m);
		while (node* const next = current->next.get()) {
			std::unique_lock<std::mutex> next_lk(next->m);
			lk.unlock();
			if (p(*next->data)) {//判断
				return next->data;
			}
			current = next;
			lk = std::move(next_lk);
		}
		return std::shared_ptr<T>();
	}

	template<typename Predicate>
	void remove_if(Predicate p) {
		node* current = &head;
		std::unique_lock<std::mutex> lk(head.m);
		while (node* const next = current->next.get()) {
			std::unique_lock<std::mutex> next_lk(next->m);
			if (p(*next->data)) {
				std::unique_ptr<node> old_next = std::move(current->next);//取出前驱结点的后继指针
				current->next = std::move(next->next);//修改前驱结点的后继指针
				next_lk.unlock();
			}
			else {
				lk.unlock();
				current = next;
				lk = std::move(next_lk);
			}
		}
	}
};