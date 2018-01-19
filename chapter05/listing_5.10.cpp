#include <string>
#include <thread>
#include <atomic>
#include <assert.h>

struct X {
	int i;
	std::string s;
};

std::atomic<X*> p;
std::atomic<int> a;

void create_x() {
	X* x = new X;
	x->i = 42;
	x->s = "hello";
	a.store(99, std::memory_order_relaxed);//1
	p.store(x, std::memory_order_release);//2
}

void use_x() {
	X* x;
	while (!(x = p.load(std::memory_order_consume)))//3
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	assert(x->i == 42);//4 no
	assert(x->s == "hello");//5 no
	assert(a.load(std::memory_order_relaxed) == 99);//possible
}

int main() {
	std::thread t1(create_x);
	std::thread t2(use_x);
	t1.join();
	t2.join();
}