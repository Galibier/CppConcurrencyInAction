#include <future>
#include <thread>
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>
#include <functional>
#include <utility>
//using namespace std;

void doSomething(std::promise<std::string> &p) {
	try {
		std::cout << "Read char ('x' for exception): ";
		char c = std::cin.get();
		if (c == 'x') {
			throw std::runtime_error(std::string("char ") + c + " read");
		}

		std::string s = std::string("char ") + c + " processed";
		p.set_value_at_thread_exit(std::move(s));
	}
	catch (...) {
		p.set_exception_at_thread_exit(std::current_exception());
	}
}

int main() {
	try {
		std::promise<std::string> p;
		std::future<std::string> f(p.get_future());

		std::thread t(doSomething, std::ref(p));
		t.detach();

		std::cout << "result: " << f.get() << std::endl;
	}
	catch (const std::exception &e) {
		std::cerr << "EXCEPTION: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "EXCEPTION" << std::endl;
	}
}