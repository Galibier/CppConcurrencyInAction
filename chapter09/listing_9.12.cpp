#include <atomic>
#include <condition_variable>
#include <mutex>

thread_local interrupt_flag this_thread_interrupt_flag;

class interrupt_flag {
	std::atomic<bool> flag;
	std::condition_variable* thread_cond;
	std::condition_variable_any* thread_cond_any;
	std::mutex set_clear_mutex;

public:
	interrupt_flag() :thread_cond(0), thread_cond_any(0) {}
	void set() {
		flag.store(true, std::memory_order_relaxed);
		std::lock_guard<std::mutex> lk(set_clear_mutex);
		if (thread_cond) {
			thread_cond->notify_all();
		}
		else if (thread_cond_any) {
			thread_cond_any->notify_all();
		}
	}
	bool is_set() const {
		return flag.load(std::memory_order_relaxed);
	}

	void set_condition_variable(std::condition_variable& cv) {
		std::lock_guard<std::mutex> lk(set_clear_mutex);
		thread_cond = &cv;
	}

	void clear_condition_variable() {
		std::lock_guard<std::mutex> lk(set_clear_mutex);
		thread_cond = 0;
	}

	struct clear_cv_on_destruct {
		~clear_cv_on_destruct() {
			this_thread_interrupt_flag.clear_condition_variable();
		}
	};

	template <typename Lockable>
	void wait(std::condition_variable_any& cv, Lockable& lk) {
		struct custom_lock {
			interrupt_flag* self;
			Lockable& lk;

			custom_lock(interrupt_flag* self_, std::condition_variable_any& cond, Lockable& lk_)
				:self(self_), lk(lk_) {
				self->set_clear_mutex.lock();
				// 1 自定义的锁类型在构造的时候，需要所锁住内部set_clear_mutex
                self->thread_cond_any = &cond; 
                // 2 对thread_cond_any指针进行设置
                //引用std::condition_variable_any 传入锁的构造函数中
            }
            // 3 当条件变量调用自定义锁的unlock()函数中的wait()时，
            //就会对Lockable对象和set_clear_mutex（mutex）进行解锁

			void unlock() {
				lk.unlock();
				self->set_clear_mutex.unlock();
			}

			void lock() {
				std::lock(self->set_clear_mutex, lk);
                /* 4 当wait()结束等待(因为等待，或因为伪苏醒)，
                *因为线程将会调用lock()函数，
                *这里依旧要求锁住内部set_clear_mutex，并且锁住Lockable对象
                */
			}

			~custom_lock() {
				self->thread_cond_any = 0;
				/* 5 在wait()调用时，custom_lock的析构函数中
                *清理thread_cond_any指针(同样会解锁set_clear_mutex)之前，
                *可以再次对中断进行检查。
                */
				self->set_clear_mutex.unlock();
			}
		};

		custom_lock cl(this, cv, lk);//自定义锁，cv可以与其它锁一起工作
		interruption_point();
		cv.wait(cl);
		interruption_point();
	}
};

template<typename Lockable>
void interruptible_wait(std::condition_variable& cv, Lockable& lk) {
	this_thread_interrupt_flag.wait(cv, lk);
}