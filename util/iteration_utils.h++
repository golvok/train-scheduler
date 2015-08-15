
#ifndef UTIL__ITERATION_UTILS_H
#define UTIL__ITERATION_UTILS_H

#include <util/utils.h++>

#include <initializer_list>
#include <iterator>
#include <array>

/*************
 * Begin definition of Iterator Pair Adapter. See end for usage
 *************/

template<typename PAIR_TYPE>
class iterator_pair_adapter {
	PAIR_TYPE t;
public:
	iterator_pair_adapter(const PAIR_TYPE& t) : t(t) { }
	iterator_pair_adapter(const iterator_pair_adapter& src) : t(src.t) { }
	iterator_pair_adapter(PAIR_TYPE&& t) : t(std::move(t)) { }
	iterator_pair_adapter(iterator_pair_adapter&& src) : t(std::move(src.t)) { }

	iterator_pair_adapter& operator=(const iterator_pair_adapter& src) { t = src.t; }
	iterator_pair_adapter& operator=(iterator_pair_adapter&& src) { t = std::move(src.t); }

	auto begin() -> decltype(t.first) { return t.first; }
	auto end() -> decltype(t.second) { return t.second; }
};

/**
 * takes a pair of iterators, and lets you iterate between them with the
 * for-range syntax
 * Example:
 *
 * std::pair<iterator_type> it_pair = get_begin_and_end()
 * for (auto it : make_iterable(it_pair)) {
 *     std::cout << *it << '\n';
 * }
 */
template<typename PAIR_TYPE>
iterator_pair_adapter<typename std::remove_reference<PAIR_TYPE>::type>
make_iterable(PAIR_TYPE&& src) {
	return iterator_pair_adapter<PAIR_TYPE>(std::forward<PAIR_TYPE>(src));
}

/*************
 * Begin definition of Index Associative Iterator. See end for usage
 *************/

template<typename IA_ITERATOR>
class index_associative_iteratior_value {
	const IA_ITERATOR* src;
public:
	typedef typename IA_ITERATOR::value_type value_type;

	typename IA_ITERATOR::iterator_type it() const { return src->it(); }
	typename IA_ITERATOR::size_type i() const { return src->i(); }

	index_associative_iteratior_value(const IA_ITERATOR& src) : src(&src) {}
	index_associative_iteratior_value(const index_associative_iteratior_value& src) = default;

	index_associative_iteratior_value& operator=(const index_associative_iteratior_value&) = delete;

	operator value_type() { return *(it()); }
	value_type& operator*() { return *(it()); }
};

template<typename ITERATOR, typename INDEX>
class index_associative_iteratior {
public:
	typedef ITERATOR iterator_type;
	typedef INDEX size_type;
	typedef index_associative_iteratior_value<index_associative_iteratior> value_type;
private:
	iterator_type iter;
	size_type index;
public:
	index_associative_iteratior(const iterator_type& it) : iter(it), index(0) {}

	index_associative_iteratior(const index_associative_iteratior&) = default;
	index_associative_iteratior& operator=(const index_associative_iteratior&) = delete;

	index_associative_iteratior& operator++() {
		++iter;
		++index;
		return *this;
	}

	bool operator==(const index_associative_iteratior& rhs) const { return iter == rhs.iter; }
	bool operator!=(const index_associative_iteratior& rhs) const { return iter != rhs.iter; }
	value_type operator*() const { return value_type(*this); }

	iterator_type it() const { return iter; }
	size_type i() const { return index; }
};

template<typename CONAINER>
class index_associative_iteratior_adapter {
	CONAINER& c;
public:
	typedef typename CONAINER::size_type size_type;
	typedef index_associative_iteratior<typename CONAINER::iterator, size_type> iterator_type;

	index_associative_iteratior_adapter(CONAINER& c) : c(c) {}
	index_associative_iteratior_adapter(const index_associative_iteratior_adapter& src) = default;

	index_associative_iteratior_adapter& operator=(const index_associative_iteratior_adapter&) = delete;

	iterator_type begin() { return iterator_type(std::begin(c)); }
	iterator_type end() { return iterator_type(std::end(c)); }
};

/**
 * Allows for iteration using the for-range syntax, but also gives you an id
 * to use. Useful for using the index as the key for a map, or into another vector.
 *
 * Example:
 * std::vector<int> some_ints { 1, 5, 7, 8, 1, };
 * for (auto iter : index_assoc_iterate(some_ints)) {
 *     std::cout << "There's a " *iter.it() << " at " << iter.i << '\n';
 * }
 *
 * Example's Output:
 * There's a 1 at 0
 * There's a 5 at 1
 * There's a 7 at 2
 * There's a 8 at 3
 * There's a 1 at 4
 *
 * Future Directions:
 * Maybe make it store a reference to the container, and just access it when
 * we need it() ? Won't work well for std::list, though.
 */
template<typename CONAINER>
index_associative_iteratior_adapter<CONAINER> index_assoc_iterate(CONAINER& c) {
	return index_associative_iteratior_adapter<CONAINER>(c);
}

#endif /* UTIL__ITERATION_UTILS_H */
