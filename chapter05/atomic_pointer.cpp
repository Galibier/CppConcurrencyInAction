#include <atomic>
#include <iostream>
#include <cassert>
class Foo {};
// fetch_add()和fetch_sub()的返回值与算数符号运算的返回值略有不同
int main() {
	Foo some_array[5];
	std::atomic<Foo*> p(some_array);
	Foo* x = p.fetch_add(2); // p加2，并返回原始值
	//assert(x == some_array);
	std::cout << x << "\t" << some_array << std::endl;
	assert(p.load() == &some_array[2]);
	x = (p -= 1); // p减1，并返回p值
	std::cout << x << "\t" << p << std::endl;
	assert(x == &some_array[1]);
	assert(p.load() == &some_array[1]);
	system("pause");
	return 0;
}