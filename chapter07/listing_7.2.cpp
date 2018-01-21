#include <atomic>

template <typename T>
class lock_free_stack {
private:
	struct node {
		T data;
		node* next;
		node(T const& data_) :data(data_) {}
	};
	std::atomic<node*> head;

public:
	void push(T const& data) {
		node* const new_node = new node(data);
		new_node->next = head.load();
		while (!head.compare_exchange_weak(new_node->next, new_node))
			;
		/*当new_node->next与head相等时，head=new_node，
        *返回true（可能直接由于平台原因失败，而非strong版本的反复尝试）；
        *不等时，new_node->next=head（完美修改了新节点指针域）
        *返回false，再次进行尝试。
        */
	}
	//1、当头指针为空的时候，程序将解引用空指针，这是未定义行为；
	//2、异常安全（最后复制栈的数据时，若抛出异常，栈数据已经丢失，可以使用shared_ptr<>解决）。
	void pop(T& result){
        node* old_head = head.load();
        while (!head.compare_exchange_weak(old_head, old_head->next));
        result = old_head->data;
    }
};

/*compare-and-swap (CAS) :In short, it loads the value from memory, compares
 it with an expected value, and if equal, store a predefined desired value to
 that memory location. The important thing is that all these are performed in
 an atomic way, in the sense that if the value has been changed in the meanwhile
 by another thread, CAS will fail.
  the weak version will return false even if the value of the object is equal 
to expected, which won’t be synced with the value in memory in this case.
*/