#include <thread>

class interrupt_flag {
public:
	void set();
	bool is_set() const;
};
thread_local interrupt_flag this_thread_interrupt_flag;

class interruptible_thread {
	std::thread internal_thread;
	interrupt_flag* flag;
public:
	template<typename FunctionType>
	interruptible_thread(FunctionType f) {
		std::promise<interrupt_flag*> p;// 2 线程将会持有f副本和本地promise变量(p)的引用
		// 3 提供函数f是包装了一个lambda函数，让线程能够调用提供函数的副本
		//lambda函数设置promise变量的值到this_thread_interrupt_flag(在thread_local中声明)的地址中
		internal_thread = std::thread([f, &p] {p.set_value(&this_thread_interrupt_flag); f(); });
		//等待就绪，结果存入到flag成员变量中
		flag = p.get_future().get();
	}
	void interrupt() {//需要一个线程去做中断时，需要一个合法指针作为一个中断标志
		if (flag) {
			flag->set();
		}
	}
};
