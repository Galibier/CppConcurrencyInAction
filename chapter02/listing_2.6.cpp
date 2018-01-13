#include <thread>
#include <utility>
#include <stdexcept>

class scope_thread {
	std::thread t;
public:
	explicit scope_thread(std::thread t_) 
		:t(std::move(t_)) {
		if (!t.joinable())
			throw std::logic_error("No thread");
	}
	~scope_thread() {
		t.join();
	}
	scope_thread(scope_thread const &) = delete;
	scope_thread& operator=(scope_thread const &) = delete;
};

void do_something(int &i) {
	++i;
}

struct func {
	int &i;
	func(int &i_) :i(i_) {}

	void operator()() {
		for (unsigned j = 0; j < 1000000; j++) {
			do_something(i);
		}
	}
};

void do_something_in_current_thread() {}

void f() {
	int some_local_state;
	scope_thread t(std::thread(func(some_local_state)));

	do_something_in_current_thread();
}

int main() {
	f();
}