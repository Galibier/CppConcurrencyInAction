#include <future>
#include <list>
#include <iostream>
#include <exception>
using namespace std;

void task1(){
	list<int> v;
	while (true) {
		for (int i = 0; i<1000000; ++i) {
			v.push_back(i);
		}
		cout.put('.').flush();
	}
}

int main(){
	cout << "starting 2 tasks" << endl;
	cout << "- task1: process endless loop of memory consumption" << endl;
	cout << "- task2: wait for <return> and then for task1" << endl;

	auto f1 = async(task1);

	cin.get();

	cout << "\nwait for the end of task1: " << endl;
	try {
		f1.get();
	}
	catch (const exception& e) {
		cerr << "EXCEPTION: " << e.what() << endl;
	}
}