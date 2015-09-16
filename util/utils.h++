#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <cstdint>
#include <memory>
#include <type_traits>

template <typename...> struct all_convertible;

template <> struct all_convertible<> : std::true_type { };

template <typename T> struct all_convertible<T> : std::true_type { };

template <typename T, typename V, typename... Rest> struct all_convertible<T, V, Rest...>
: std::integral_constant<
	bool,
	std::is_convertible<V, T>::value && all_convertible<T,Rest...>::value
> { };

namespace util {
	template<typename T>
	void reverse(T& t) {
		reverse(std::begin(t),std::end(t));
	}

	template<typename FUNC>
	void repeat(uint count, const FUNC& f) {
		for (uint i = 1; i <= count; ++i) {
			f();
		}
	}
	template<typename T>
	auto make_shared(T&& t) {
		return std::make_shared<
			typename std::remove_cv<
				typename std::remove_reference<T>::type
			>::type
		>(
			std::forward<T>(t)
		);
	}
}

/*******
 * Begin definition of ComparesWithTag
 *******/

template<typename T, typename U>
class ComparesWithTag {
	const T& thing;
	U identifier;

public:
	ComparesWithTag(const T& thing, U identifier)
		: thing(thing)
		, identifier(identifier)
	{ }
	ComparesWithTag(const ComparesWithTag&) = default;
	ComparesWithTag& operator=(const ComparesWithTag&) = delete;
	ComparesWithTag& operator=(ComparesWithTag&&) = default;

	operator std::tuple<T,U>() {
		return {thing,identifier};
	}

	template<typename THEIR_T, typename THEIR_U>
	bool operator<(const ComparesWithTag<THEIR_T,THEIR_U> rhs) const {
		return thing < rhs.thing;
	}

	const T& value() { return thing; }
	U id() { return identifier; }
};


/**
 * Allows you to tag some data along when you call std::max or something.
 * Defines operator< using the first argumen's operator<
 *
 * Example:
 * 	  enum class MyEnum {
 *        A,B,
 *    };
 *
 *    auto result = std::max(
 *        compare_with_tag(getA(),MyEnum::A),
 *        compare_with_tag(getB(),MyEnum::B)
 *    );
 *
 *    if (result.id() == MyEnum::A) {
 *        std::cout << "A was smaller, A = " << result << '\n';
 *    } else if (result.id() == MyEnum::B) {
 *        std::cout << "B was smaller, B = " << result << '\n';
 *    }
 */
template<typename T, typename U>
ComparesWithTag<T,U> compare_with_tag(const T& thing, U id) {
	return ComparesWithTag<T,U>(thing,id);
}

template<typename T, typename V = void>
using EnableIfEnum = typename std::enable_if<std::is_enum<T>::value,V>;

template<typename T, typename V = void>
using EnableIfIntegral = typename std::enable_if<std::is_integral<T>::value,V>;

#endif /* UTIL_H */
