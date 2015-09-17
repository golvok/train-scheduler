
#ifndef UTIL__LOGGING_H
#define UTIL__LOGGING_H

#include <util/utils.h++>

#include <bitset>
#include <boost/iostreams/filtering_stream.hpp>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

class IndentingLeveledDebugPrinter;

namespace DebugLevel {
	/**
	 * The Enum of the levels usable with IndentingLeveledDebugPrinter
	 * If adding a level make sure to update the various functions in this namespace.
	 */
	enum Level : uint {
		INFO,  // probably always going to have this on?
		WARN,  // same as ^ ?
		ERROR, // always on

		WC_D1, // Wanted capacity debug
		WC_D2, // Wanted capacity debug level 2
		WC_D3, // Wanted capacity lowest level debug
		TR_D1, // Train Routing debug
		TR_D2, // Train Routing debug level 2
		TR_D3, // Train Routing lowest level debug
		PR_D1, // Passenger Routing debug
		PR_D2, // Passenger Routing debug level 2
		PR_D3, // Passenger Routing lowest level debug

		LEVEL_COUNT, // please make sure this is at the end
	};

	/**
	 * Get the default set of print levels that should probably always be enabled
	 * Most code assumes these are already on.
	 */
	std::vector<Level> getDefaultSet();

	/**
	 * Get all the levels that you might ever want if debugging was your goal
	 */
	std::vector<Level> getAllDebug();

	/**
	 * If you feel like enabling a particular level, then
	 * call this function to get all the levels that probably
	 * should also be enabled
	 */
	std::vector<Level> getAllShouldBeEnabled(Level l);

	std::pair<Level,bool> getFromString(std::string str);
	std::string getAsString(Level l);

}

using DL = DebugLevel::Level;

template<
	typename STREAM_TYPE,
	typename REDIRECT_TYPE,
	typename LEVEL_TYPE,
	size_t NUM_LEVELS
>
class LevelRedirecter {
private:
	std::bitset<NUM_LEVELS> enabled_levels;

	STREAM_TYPE* cast_to_str() { return static_cast<STREAM_TYPE*>(this); }
public:
	LevelRedirecter()
		: enabled_levels()
	{ }

	virtual ~LevelRedirecter() = default;

	LevelRedirecter(const LevelRedirecter&) = delete;
	LevelRedirecter& operator=(const LevelRedirecter&) = delete;

	LevelRedirecter(LevelRedirecter&&) = default;
	LevelRedirecter& operator=(LevelRedirecter&&) = default;

	REDIRECT_TYPE operator()(const LEVEL_TYPE& level) {
		if (enabled_levels.test(level)) {
			return REDIRECT_TYPE(cast_to_str());
		} else {
			return REDIRECT_TYPE(nullptr);
		}
	}

	template<typename T>
	void enable_level(const T& level) {
		set_enable_for_level(level,true);
	}

	template<typename T>
	void disable_level(const T& level) {
		set_enable_for_level(level,false);
	}

	template<typename LOCAL_LEVEL_TYPE>
	void set_enable_for_level(
		const LOCAL_LEVEL_TYPE& level,
		bool enable,
		typename EnableIfEnum<LOCAL_LEVEL_TYPE>::type** = 0,
		typename std::enable_if<std::is_same<LOCAL_LEVEL_TYPE,LEVEL_TYPE>::value>::type* = 0
	) {
		enabled_levels.set(
			static_cast<std::underlying_type_t<LOCAL_LEVEL_TYPE>>(level),
			enable
		);
	}

	template<typename LOCAL_LEVEL_TYPE>
	void set_enable_for_level(
		const LOCAL_LEVEL_TYPE& level,
		bool enable,
		typename EnableIfIntegral<LOCAL_LEVEL_TYPE>::type* = 0
	) {
		enabled_levels.set(
			level,
			enable
		);
	}
};

class indent_filter	: public boost::iostreams::output_filter {
private:
	friend class IndentingLeveledDebugPrinter;

	IndentingLeveledDebugPrinter& src;
	bool just_saw_newline;

	explicit indent_filter(IndentingLeveledDebugPrinter& src)
		: src(src)
		, just_saw_newline(true)
	{ }

public:
	template<typename SINK> bool put(SINK& dest, int c);

	// this should reset the state
	template<typename SINK> void close(SINK&) { just_saw_newline = true; }
};

class IndentLevel {
	friend class IndentingLeveledDebugPrinter;
	friend class LevelStream;
	IndentingLeveledDebugPrinter* src;
	bool ended;
	IndentLevel(IndentingLeveledDebugPrinter* src) : src(src), ended(false) { }
public:
	void endIndent();
	~IndentLevel();

	IndentLevel(IndentLevel&& src_ilevel)
		: src(std::move(src_ilevel.src))
		, ended(std::move(src_ilevel.ended))
	{
		src_ilevel.ended = true;
	}

	IndentLevel(const IndentLevel&) = delete;

	IndentLevel& operator=(IndentLevel&& rhs) {
		this->src = std::move(rhs.src);
		this->ended = std::move(rhs.ended);
		rhs.ended = true;
		return *this;
	}

	IndentLevel& operator=(const IndentLevel&) = delete;
};

class LevelStream {
private:
	friend class IndentingLeveledDebugPrinter;

	IndentingLeveledDebugPrinter* src;

public:
	LevelStream(IndentingLeveledDebugPrinter* src)
		: src(src)
	{ }

	LevelStream(const LevelStream&) = default;
	LevelStream(LevelStream&&) = default;

	LevelStream& operator=(const LevelStream&) = default;
	LevelStream& operator=(LevelStream&&) = default;

	template<typename T>
	friend LevelStream& operator<<(LevelStream& lhs, const T& t);

	template<typename T>
	IndentLevel indentWithTitle(const T& t);

	bool enabled() { return src != nullptr; }
};

class IndentingLeveledDebugPrinter
	: private boost::iostreams::filtering_ostream
	, public LevelRedirecter
		< IndentingLeveledDebugPrinter
		, LevelStream
		, DebugLevel::Level
		, DebugLevel::LEVEL_COUNT
	>
{
	int max_indent_level;
	int indent_level;

	std::stringstream ss;
public:

	IndentingLeveledDebugPrinter(std::ostream& os, int max_indent_level)
		: boost::iostreams::filtering_ostream()
		, LevelRedirecter()
		, max_indent_level(max_indent_level)
		, indent_level(0)
		, ss()
	{
		push(indent_filter(*this));
		push(os);
	}

	IndentingLeveledDebugPrinter(const IndentingLeveledDebugPrinter&) = delete;
	IndentingLeveledDebugPrinter& operator=(const IndentingLeveledDebugPrinter&) = delete;

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
	auto indentWithTitle(const FUNC& f) -> decltype(f(ss),IndentLevel(this)) {
		// the weird return value is so the compiler SFINAE's away this
		// overload if FUNC is not a lambda style type
		util::repeat(getTitleLevel(),[&](){
			print('=');
		});
		print(' ');

		f(ss);
		std::string title = ss.str();
		print(title);
		std::stringstream().swap(ss); // empty the stringstream

		print(' ');
		util::repeat(getTitleLevel(),[&](){
			print('=');
		});
		print('\n');
		indent_level++;
		return IndentLevel(this);
	}

	IndentLevel indentWithTitle(const std::string& title) {
		return indentWithTitle([&](auto&& s){ s << title; });
	}

	IndentLevel indentWithTitle(const char*& title) {
		return indentWithTitle([&](auto&& s){ s << title; });
	}

	void setMaxIndentation(int level) { max_indent_level = level; }
private:
	friend class IndentLevel;
	void endIndent() {
		if (indent_level > 0) {
			indent_level = indent_level - 1;
		}
	}
};

extern IndentingLeveledDebugPrinter dout;

template<typename T>
LevelStream& operator<<(LevelStream&& lhs, const T& t) {
	return lhs << t;
}

template<typename T>
LevelStream& operator<<(LevelStream& lhs, const T& t) {
	if (lhs.enabled()) {
		lhs.src->print(t);
	}
	return lhs;
}

template<typename T>
IndentLevel LevelStream::indentWithTitle(const T& t) {
	if (enabled()) {
		return src->indentWithTitle(t);
	} else {
		return IndentLevel(nullptr);
	}
}

#endif /* UTIL__LOGGING_H */
