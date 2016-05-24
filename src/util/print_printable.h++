
#ifndef UTIL__PRINT_PRINTABLE_HPP
#define UTIL__PRINT_PRINTABLE_HPP

namespace util {

// TODO: maybe provide some sort of default? maybe one that gives a warning when compiled?
struct print_printable { };

template<typename T, typename STREAM>
auto operator<<(STREAM& os, const T& t) -> decltype(static_cast<const print_printable*>(&t),os) {
	t.print(os);
	return os;
}

} // end namespace util

#endif /* UTIL__PRINT_PRINTABLE_HPP */
