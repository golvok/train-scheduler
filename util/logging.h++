
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

/**
 * This class supplies REDIRECT_TYPE operator()(LEVEL_TYPE) and
 * methods for manipulating the enable/disable state of each level
 *
 * Classes that use this are expected to inherit from this class,
 * so that the level manipulation & operator() usage is seamless.
 * It is not necessary, however.
 *
 * Template Arguments:
 * STREAM_GET_TYPE - something that provides operator()(LevelRedirecter*)
 *     which converts it's argument to something that can be passed
 *     to the constructor of REDIRECT_TYPE
 * REDIRECT_TYPE - the return value of the operator()(LEVEL_TYPE). Must
 *     be constructable from the return value of STREAM_GET_TYPE()(LevelRedirecter*)
 * LEVEL_TYPE - the type that should be passed to operator()(LEVEL_TYPE)
 * NUM_LEVELS - the total number & one plus the maximum of the levels that will be usable
 *     The enable/disable functions will throw exceptions if called with something
 *     equal or greater to this.
 */
template<
	typename STREAM_GET_TYPE,
	typename REDIRECT_TYPE,
	typename LEVEL_TYPE,
	size_t NUM_LEVELS
>
class LevelRedirecter {
private:
	std::bitset<NUM_LEVELS> enabled_levels;

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
			return REDIRECT_TYPE(STREAM_GET_TYPE()(this));
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

/**
 * The class that detects lines and inserts the tabs to indent the lines
 * properly
 */
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

/**
 * A little helper class that is returned when an indent is done
 * It will either unindent when its destructor is called, or endIndent() is called
 */
class IndentLevel {
	friend class IndentingLeveledDebugPrinter;
	friend class LevelStream;
	IndentingLeveledDebugPrinter* src;
	bool ended;
	IndentLevel(IndentingLeveledDebugPrinter* src) : src(src), ended(false) { }
public:
	void endIndent();
	~IndentLevel();

	/**
	 * Move Constructor - disable the old one, but otherwise copy everything over
	 */
	IndentLevel(IndentLevel&& src_ilevel)
		: src(std::move(src_ilevel.src))
		, ended(std::move(src_ilevel.ended))
	{
		src_ilevel.ended = true;
	}

	/// copying would cause problems - who would do the unindent?
	IndentLevel(const IndentLevel&) = delete;

	/**
	 * Move Assignment - disable the old one, but otherwise copy everything over
	 */
	IndentLevel& operator=(IndentLevel&& rhs) {
		this->src = std::move(rhs.src);
		this->ended = std::move(rhs.ended);
		rhs.ended = true;
		return *this;
	}

	/// same problems as copying
	IndentLevel& operator=(const IndentLevel&) = delete;
};

/**
 * A helper class for actually printing. IndentingLeveledDebugPrinter doesn't
 * actually have operator<< defined, so that you have to use operator() to print.
 * (it returns an object of this class)
 */
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

/**
 * The class that handles most of the output control & formatting - the type of dout
 * It inherits from LevelRedirecter to define operator()(Debug::Level) and the enable/disable
 * controls. It also inherits from a filtering_ostream to allow filtering of the output.
 *
 * To print something, you must do dout(DL::*) << "string", and the same must be done for
 * an indent level.
 *
 * The max_indent_level controls how many '=' to put around the title of the indent level at
 * the default level. Each inner level will be indented one tab stop, and have one fewer '=' on
 * each side of the title's text - with a minimum of one.
 */
class IndentingLeveledDebugPrinter
	: private boost::iostreams::filtering_ostream
	, public LevelRedirecter
		< StaticCaster<IndentingLeveledDebugPrinter*>
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
		this->flush(); // force printing now, so that indent is for sure correct.
		ss.clear(); // clear EOF bit
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

// has to be down here because the type of src (IndentingLeveledDebugPrinter*)
// hadn't been defined yet, so it needed be be after it to call it's methods
template<typename T>
IndentLevel LevelStream::indentWithTitle(const T& t) {
	if (enabled()) {
		return src->indentWithTitle(t);
	} else {
		return IndentLevel(nullptr);
	}
}

#endif /* UTIL__LOGGING_H */
