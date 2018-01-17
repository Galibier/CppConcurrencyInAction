#include <list>
#include <future>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>

class join_threads {
	std::vector<std::thread>& threads;
public:
	explicit join_threads(std::vector<std::thread>& threads_) :threads(threads_) {}
	~join_threads() {
		for (unsigned long i = 0; i < threads.size(); ++i) {
			if (threads[i].joinable())
				threads[i].join();
		}
	}
};

class barrier {
	std::atomic<unsigned> count;
	std::atomic<unsigned> spaces;
	std::atomic<unsigned> generation;
public:
	explicit barrier(unsigned count_) : count(count_), spaces(count_), generation(0) {}
	void wait() {
		unsigned const my_generation = generation;
		if (!--spaces) {
			spaces = count.load();
			++generation;
		}
		else {
			while (generation == my_generation)
				std::this_thread::yield();
		}
	}
	void done_waiting() {
		--count;
		if (!--spaces) {
			spaces = count.load();
			++generation;
		}
	}
};

template <typename Iterator, typename T>
void parallel_partial_sum(Iterator first, Iterator last) {
	typedef typename Iterator::value_type value_type;
	struct process_element {
		void operator()(Iterator begin, Iterator last, std::vector<value_type>& buffer, unsigned i, barrier& b) {
			value_type& ith_element = *(first + i);
			bool updata_source = false;
			for (unsigned step = 0, stride = 1; stride <= i; ++step, stride *= 2) {
				value_type const source = (step % 2) ? buffer[i] : ith_element;
				value_type& dest = (step % 2) ? ith_element : buffer[i];
				value_type const addend = (step % 2) ? buffer[i - stride] : *(first + i - stride);
				dest = source + addend;
				updata_source = !(step % 2);
				b.wait();
			}
			if (updata_source) {
				ith_element = buffer[i];
			}
			b.done_waiting();
		}
	};
	unsigned long const length = std::distance(first, last);
	if (length <= 1)
		return;

	std::vector<value_type> buffer(length);
	barrier b(length);
	std::vector<std::thread> threads(length - 1);
	join_threads joiner(threads);

	Iterator block_start = first;
	for (unsigned long i = 0; i < (length - 1); ++i) {
		threads[i] = std::thread(process_element(), first, last, std::ref(buffer), i, std::ref(b));
	}
	process_element()(first, last, buffer, length - 1, b);
}
