#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <list>
#include <utility>
#include <map>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread.hpp>

这个查询表作为一个整体，通过单独的操作，对每一个桶进行锁定，并且通过使用boost::shared_mutex允许读者线程对每一个桶进行并发访问。
template<typename Key, typename Value, typename Hash = std::hash<Key> >
class threadsafe_lookup_table
{
private:
	class bucket_type {
	private:
		typedef std::pair<Key, Value> bucket_value;
		typedef std::list<bucket_value> bucket_data;
		typedef typename bucket_data::iterator bucket_iterator;

		bucket_data data;//list<pair<Key,Value>>，链表中存放键值对
		mutable boost::shared_mutex mutex;// 1，对每一个桶实施保护

		bucket_iterator find_entry_for(Key const& key) const {// 2，确定key是否在桶中
			return std::find_if(data.begin(), data.end(), [&](bucket_value const& item) {return item.first == key; });
		}
	public:
		Value value_for(Key const& key, Value const& default_value) const {
			boost::shared_lock<boost::shared_mutex> lock(mutex);// 3，共享(只读)所有权
			bucket_iterator const found_entry = find_entry_for(key);
			return (found_entry == data.end()) ? default_value : found_entry->second;
		}

		void add_or_update_mapping(Key const& key, Value const& value) {
			std::unique_lock<boost::shared_mutex> lock(mutex);//4，唯一（读写）所有权
			bucket_iterator const found_entry = find_entry_for(key);
			if (found_entry == data.end()) {
				data.push_back(bucket_value(key, value));
			}
			else {
				found_entry->second = value;
			}
		}

		void remove_mapping(Key const& key) {
			std::unique_lock<boost::shared_mutex> lock(mutex);//// 5，唯一(读/写)权
			bucket_iterator const found_entry = find_entry_for(key);
			if (found_entry != data.end()) {
				data.erase(found_entry);
			}
		}
	};

	std::vector<std::unique_ptr<bucket_type> > buckets;//保存桶
	Hash hasher;//哈希函数

	bucket_type& get_bucket(Key const& key) const {//可以无锁调用
		std::size_t const bucket_index = hasher(key) % buckets.size();
		return *buckets[bucket_index];
	}

public:
	typedef Key key_type;
	typedef Value mapped_type;
	typedef Hash hash_type;

	threadsafe_lookup_table(unsigned num_buckets = 19, Hash const& hasher_ = Hash())//默认桶的数量，任意质数，哈希表在有质数个桶时，工作效率最高。
		:buckets(num_buckets), hasher(hasher_) {
		for (unsigned i = 0; i < num_buckets; ++i)
		{
			buckets[i].reset(new bucket_type);
		}
	}

	threadsafe_lookup_table(threadsafe_lookup_table const& other) = delete;
	threadsafe_lookup_table& operator=(threadsafe_lookup_table const& other) = delete;

	Value value_for(Key const& key, Value const& default_value = Value()) const {
		return get_bucket(key).value_for(key, default_value);
	}

	void add_or_update_mapping(Key const& key, Value const& value) {
		get_bucket(key).add_or_update_mapping(key, value);
	}

	void remove_mapping(Key const& key) {
		get_bucket(key).remove_mapping(key);
	}

	/***********************************************************************************************/
	//查询表的一个“可有可无”(nice-to-have)的特性，会将选择当前状态的快照
	std::map<Key, Value> get_map() const {
		std::vector<std::unique_lock<boost::shared_mutex> > locks;//锁的数组
		for (unsigned i = 0; i < buckets.size(); ++i) {
			locks.push_back(std::unique_lock<boost::shared_mutex>(buckets[i].mutex));//上锁后压入
		}
		std::map<Key, Value> res;
		for (unsigned i = 0; i < buckets.size(); ++i) {
			for (bucket_iterator it = buckets[i].data.begin(); it != buckets[i].data.end(); ++it) {
				res.insert(*it);//存入键值对
			}
		}
		return res;
	}
	/**********************************************************************************************/

};

