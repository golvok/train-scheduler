#include "thread_utils.h++"

#include <algorithm>

namespace util {

SafeWaitForNotify::SafeWaitForNotify()
	: num_waiting(0)
	, waiting_threads()
	, cv_lock()
	, cv()
	, dying(false)
	, dying_cv()
{ }

/**
 * (permanently) Disables this class, and causes all waiting threads to exit wait()
 * and any threads that call wait() while waiting for them to leave to not wait at all.
 */
SafeWaitForNotify::~SafeWaitForNotify() {
	std::unique_lock<std::mutex> cv_ul(cv_lock);

	dying = true; // prevent new waiters, and flag current ones
	notify_all_nolock(); // wake up everyone currently waiting
	dying_cv.wait(cv_ul, [&] {
		// wait until no threads
		return num_waiting == 0;
	});
}

void SafeWaitForNotify::notify_all() {
	std::unique_lock<std::mutex> cv_ul(cv_lock);
	notify_all_nolock();
}

void SafeWaitForNotify::wait() {
	std::unique_lock<std::mutex> cv_ul(cv_lock);
	if (dying) return; // this is currently being destructed
	auto thread_id = std::this_thread::get_id();

	// add self to waiting threads list
	waiting_threads.push_back(thread_id);

	cv.wait(cv_ul, [&] {
		auto begin = waiting_threads.begin();
		auto end = waiting_threads.begin() + num_waiting;
		auto find_result = std::find(begin, end, thread_id);
		if (find_result != end) {
			// found the current thread in the first num_waiting threads

			// remove self
			waiting_threads.erase(find_result,find_result+1);

			// we just erased an element, so this needs to be updated
			num_waiting -= 1;

			// check if this is being destructed and is ready to be
			if (dying && num_waiting == 0) {
				dying_cv.notify_all();
			}

			return true; // exit std::thread::wait
		} else {
			return false; // spurious wakeup/wasn't waiting
		}
	});
}

/**
 * The version of notify_all() that doesn't try to lack the mutex... make sure it is owned
 * when calling this
 */
void SafeWaitForNotify::notify_all_nolock() {
	// record number of waiting threads
	num_waiting = waiting_threads.size();
	cv.notify_all();
}

} // end namespace until
