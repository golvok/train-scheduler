#ifndef UTIL_H
#define UTIL_H

#include "track_network.h++"
#include <string>
#include <algorithm>
#include <iostream>
#include <boost/iostreams/filtering_stream.hpp>
#include <sstream>
#include <cstdint>

class Passenger {
	std::string name;
	TrackNetwork::ID entry_id;
	TrackNetwork::ID exit_id;
	uint start_time;

public:
	Passenger(
		const std::string name,
		TrackNetwork::ID entry_id,
		TrackNetwork::ID exit_id,
		uint start_time
	)
		: name(name)
		, entry_id(entry_id)
		, exit_id(exit_id)
		, start_time(start_time)
	{}

	const std::string& getName() const { return name; }
	TrackNetwork::ID getEntryId() const { return entry_id; }
	TrackNetwork::ID getExitId() const { return exit_id; }
	uint getStartTime() const { return start_time; }

	bool operator==(const Passenger& rhs) const {
		return name == rhs.name;
	}
};

namespace std {
	template<>
	struct hash<Passenger> {
		size_t operator()(const Passenger& p) const {
			return std::hash<std::string>()(p.getName());
		}
	};
}

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
}

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
	CONAINER* c;
public:
	typedef typename CONAINER::size_type size_type;
	typedef index_associative_iteratior<typename CONAINER::iterator, size_type> iterator_type;

	index_associative_iteratior_adapter(CONAINER& c) : c(&c) {}
	index_associative_iteratior_adapter(const index_associative_iteratior_adapter& src) = default;

	index_associative_iteratior_adapter& operator=(const index_associative_iteratior_adapter&) = delete;

	iterator_type begin() { return iterator_type(std::begin(*c)); }
	iterator_type end() { return iterator_type(std::end(*c)); }
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

/********
 * Begin definition of Indenting Debug Printer
 ********/

class IndentingDebugPrinter;

class indent_filter	: public boost::iostreams::output_filter {
private:
	friend class IndentingDebugPrinter;

	IndentingDebugPrinter& src;
	bool just_saw_newline;

	explicit indent_filter(IndentingDebugPrinter& src)
		: src(src)
		, just_saw_newline(true)
	{ }

public:
	template<typename SINK> bool put(SINK& dest, int c);

	// this should reset the state
	template<typename SINK> void close(SINK&) { just_saw_newline = true; }
};

class IndentLevel {
	friend class IndentingDebugPrinter;
	IndentingDebugPrinter& src;
	bool ended;
	IndentLevel(IndentingDebugPrinter& src) : src(src), ended(false) { }
public:
	void endIndent();
	~IndentLevel();
};

class IndentingDebugPrinter : boost::iostreams::filtering_ostream {
	friend class IndentLevel;
	int max_indent_level;
	int indent_level;

	std::stringstream ss;
public:
	IndentingDebugPrinter(std::ostream& os, int max_indent_level)
		: max_indent_level(max_indent_level)
		, indent_level(0)
		, ss()
	{
		push(indent_filter(*this));
		push(os);
	}

	IndentingDebugPrinter(const IndentingDebugPrinter&) = delete;
	IndentingDebugPrinter& operator=(const IndentingDebugPrinter&) = delete;

	template<typename T>
	void print(const T& t) {
		ss << t;
		while (true) {
			auto c = ss.get();
			if (ss.eof()) { break; }
			this->put(c);
		}
		this->flush(); // force priting now, so that indent is for sure correct.
		ss.clear(); // clear eof bit
	}

	uint getIndentLevel() {
		return indent_level;
	}

	uint getNumSpacesToIndent() {
		return indent_level * 2;
	}

	uint getTitleLevel() {
		if (indent_level >= max_indent_level) {
			return 1;
		} else {
			return max_indent_level-indent_level;
		}
	}

	template<typename FUNC>
	auto indentWithTitle(const FUNC& f) -> decltype(f(*this),IndentLevel(*this)) {
		// the weird return value is so the compiler SFINAE's away this
		// override if FUNC is not a lambda style type
		util::repeat(getTitleLevel(),[&](){
			(*this) << '=';
		});
		(*this) << ' ';
		f(*this);
		(*this) << ' ';
		util::repeat(getTitleLevel(),[&](){
			(*this) << '=';
		});
		(*this) << '\n';
		indent_level++;
		return IndentLevel(*this);
	}

	IndentLevel indentWithTitle(const std::string& title) {
		return indentWithTitle([&](auto& s){ s << title; });
	}

	IndentLevel indentWithTitle(const char*& title) {
		return indentWithTitle([&](auto& s){ s << title; });
	}

	void setMaxIndentation(int level) { max_indent_level = level; }
private:
	void endIndent() {
		if (indent_level > 0) {
			indent_level = indent_level - 1;
		}
	}
};

template<typename T>
IndentingDebugPrinter& operator<<(IndentingDebugPrinter& lhs, const T& rhs) {
	lhs.print(rhs);
	return lhs;
}

extern IndentingDebugPrinter dout;

#endif /* UTIL_H */
