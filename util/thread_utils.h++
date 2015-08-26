
#ifndef UTILS__THREAD_UTILS_H
#define UTILS__THREAD_UTILS_H

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace util {

/**
 * A class that implements a safe notify_all, for when you don't have a
 * good precondition to deal with spurious wakups, but want to use
 * condition variable semantics
 */
class SafeWaitForNotify {
private:
public:
	SafeWaitForNotify();
	~SafeWaitForNotify();

	SafeWaitForNotify& operator=(const SafeWaitForNotify&) = delete;

	/**
	 * Wake up all threads that have called wait()
	 */
	void notify_all();

	/**
	 * Blocks thread until notify_all() is called. guranteed to never return
	 * as the result of a spurious wakeup
	 */
	void wait();

private:
	void notify_all_nolock();

	// the first num_waiting of threads in waiting_threads
	// were waiting when notify_all was called
	uint num_waiting;
	// threads waiting
	std::vector<std::thread::id> waiting_threads;

	// mutex for member variables
	std::mutex cv_lock;

	// cv used to coordinate wait and notify_all
	std::condition_variable cv;

	// signifies the destructor has been called
	bool dying;
	// destructor thread waits on this
	std::condition_variable dying_cv;
};

template<typename DATA,typename MUTEX = std::recursive_mutex>
class ScopedLockAndData {
private:
	DATA data;
	std::unique_lock<MUTEX> ul;

public:
	ScopedLockAndData(const DATA& d, MUTEX& m)
		: data(d)
		, ul(m)
	{ }

	ScopedLockAndData(const DATA& d, std::unique_lock<MUTEX>&& ul_)
		: data(d)
		, ul(std::move(ul_))
	{ }

	operator DATA() {
		return data;
	}
};

} // end namespace util

#endif /* UTILS__THREAD_UTILS_H */
