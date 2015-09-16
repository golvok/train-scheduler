
#ifndef UTIL__LOGGING_H
#define UTIL__LOGGING_H

#include <util/utils.h++>

#include <boost/iostreams/filtering_stream.hpp>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <string>

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
		return indentWithTitle([&](auto&& s){ s << title; });
	}

	IndentLevel indentWithTitle(const char*& title) {
		return indentWithTitle([&](auto&& s){ s << title; });
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

#endif /* UTIL__LOGGING_H */
