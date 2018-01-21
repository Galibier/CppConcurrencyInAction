#include <atomic>
#include <thread>
#include <stdexcept>

unsigned const max_hazard_pointers = 100;
struct hazard_pointer {
	std::atomic<std::thread::id> id;
	std::atomic<void*> pointer;
};

hazard_pointer hazard_pointers[max_hazard_pointers];//固定长度的“线程ID-指针”数组

class hp_owner {
	hazard_pointer* hp;
public:
	hp_owner(hp_owner const&) = delete;
	hp_owner operator=(hp_owner const&) = delete;
	hp_owner() :hp(nullptr) {
		for (unsigned i = 0; i < max_hazard_pointers; ++i) {//查询“所有者/指针”表，寻找没有所有者的记录
			std::thread::id old_id;
			if (hazard_pointers[i].id.compare_exchange_strong(old_id, std::this_thread::get_id())) {// 6 尝试声明风险指针的所有权
				hp = &hazard_pointers[i];
				break;// 7 交换成功，当前线程拥有，停止搜索
			}
		}
		if (!hp) {// 1 若没有找到，则很多线程在使用风险指针，报错
			throw std::runtime_error("No hazard pointers available");
		}
	}

	std::atomic<void*>& get_pointer() {
		return hp->pointer;
	}

	~hp_owner() {// 2 析构，使得记录可以被复用
		hp->pointer.store(nullptr);
		hp->id.store(std::thread::id());
	}
};

std::atomic<void*>& get_hazard_pointer_for_current_thread() {
	thread_local static hp_owner hazard;// 4 每个线程都有自己的风险指针，thread_local：本线程
	return hazard.get_pointer();
}

//在删除之前对风险指针引用的节点进行检查，是否被引用
bool outstanding_hazard_pointers_for(void* p) {
	for (unsigned i = 0; i < max_hazard_pointers; ++i) {
		if (hazard_pointers[i].pointer.load() == p) {
			return true;
		}
	}
	return false;
}