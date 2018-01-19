#include <atomic>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>

std::vector<int> data;
std::atomic<bool> data_ready(false);

void reader_thread() {
	while (!data_ready.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	std::cout << "The answer=" << data[0] << std::endl;
}

void writer_thread() {
	data.push_back(42);
	data_ready = true;
}

int main() {
	std::thread t_reader{ reader_thread }, t_writer{ writer_thread };
	t_reader.detach();
	t_writer.detach();
	system("pause");
	return 0;
}