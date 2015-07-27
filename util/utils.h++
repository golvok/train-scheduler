#ifndef UTIL_H
#define UTIL_H

#include "track_network.h++"
#include <string>
#include <algorithm>
#include <iostream>

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

template<typename ITERATOR, typename INDEX>
class index_associative_iteratior_value {
	ITERATOR iter;
	INDEX index;
public:
	ITERATOR it() { return iter; }
	INDEX i() { return index; }
	index_associative_iteratior_value(const ITERATOR& iter, const INDEX& index)
		: iter(iter), index(index) {}
	index_associative_iteratior_value(const index_associative_iteratior_value& src)
		: iter(src.iter), index(src.index) {}

	index_associative_iteratior_value& operator=(const index_associative_iteratior_value&) = default;
};

template<typename ITERATOR>
class index_associative_iteratior {
	ITERATOR iter;
	size_t index;
public:
	index_associative_iteratior(const ITERATOR& it) : iter(it), index(0) {}
	// decl copycon & op assign ?

	index_associative_iteratior& operator++() {
		++iter;
		++index;
		return *this;
	}
	bool operator==(const index_associative_iteratior& rhs) const {
		return iter == rhs.iter;
	}
	bool operator!=(const index_associative_iteratior& rhs) const {
		return !(*this == rhs);
	}

	index_associative_iteratior_value<ITERATOR,size_t> operator*() const {
		return index_associative_iteratior_value<ITERATOR,size_t>(iter,index);
	}
};

template<typename CONAINER>
class index_associative_iteratior_adapter {
	CONAINER& c;
public:
	index_associative_iteratior_adapter(CONAINER& c) : c(c) {}
	index_associative_iteratior_adapter(
		const index_associative_iteratior_adapter& src) : c(src.c) {}
	index_associative_iteratior_adapter(
		index_associative_iteratior_adapter&& src) : c(src.c) {}

	index_associative_iteratior_adapter& operator=(const index_associative_iteratior_adapter&) = delete;
	index_associative_iteratior_adapter& operator=(index_associative_iteratior_adapter&&) = delete;

	index_associative_iteratior<typename CONAINER::iterator> begin() {
		return index_associative_iteratior<typename CONAINER::iterator>(std::begin(c));
	}
	index_associative_iteratior<typename CONAINER::iterator> end() {
		return index_associative_iteratior<typename CONAINER::iterator>(std::end(c));
	}
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

class IndentLevel {
	friend class IndentingDebugPrinter;
	IndentingDebugPrinter& src;
	bool ended;
	IndentLevel(IndentingDebugPrinter& src) : src(src), ended(false) {}
public:
	void endIndent();
	~IndentLevel();
};

class IndentingDebugPrinter {
	friend class IndentLevel;
	std::ostream* out;
	int max_indent_level;
	int indent_level;
public:
	IndentingDebugPrinter(std::ostream& os, int max_indent_level)
		: out(&os)
		, max_indent_level(max_indent_level)
		, indent_level(0)
	{}

	IndentingDebugPrinter(const IndentingDebugPrinter&) = delete;
	IndentingDebugPrinter& operator=(const IndentingDebugPrinter&) = delete;

	template<typename T>
	void print(const T& t) {
		printIndent();
		(*out) << t;
	}

	void printIndent() {
		util::repeat(std::min(indent_level,max_indent_level),[&](){
			(*out) << ' ' << ' ';
		});
	}

	IndentLevel indentWithTitle(const std::string& title) {
		return indentWithTitleF([&](auto& s){ s << title; });
	}

	template<typename FUNC>
	IndentLevel indentWithTitleF(const FUNC& f) {
		uint num_equals = std::max(1,max_indent_level-indent_level);
		printIndent();
		util::repeat(num_equals,[&](){
			(*out) << '=';
		});
		(*out) << ' ';
		f(this->str());
		(*out) << ' ';
		util::repeat(num_equals,[&](){
			(*out) << '=';
		});
		(*out) << '\n';
		indent_level++;
		return IndentLevel(*this);
	}

	std::ostream& str() { return *out; }
	void setMaxIndentation(int level) { max_indent_level = level; }
private:
	void endIndent() {
		indent_level = std::max(indent_level-1,0);
	}
};

template<typename T>
std::ostream& operator<<(IndentingDebugPrinter& lhs, const T& rhs) {
	lhs.print(rhs);
	return lhs.str();
}

extern IndentingDebugPrinter dout;

#endif /* UTIL_H */
