
#ifndef PARSING__CMDARGS_PARSER_H
#define PARSING__CMDARGS_PARSER_H

#include <util/logging.h++>

namespace parsing {

namespace cmdargs {

class ParsedArguments {
public:
	/**
	 * Return the list of debug levels that shold be enabled given the command line options
	 * NOTE: May contain duplicates.
	 */
	const std::vector<DebugLevel::Level> getDebugLevelsToEnable() const {
		return levels_to_enable;
	}

	/**
	 * Should the current invocation of the program display graphics?
	 */
	bool shouldEnableGraphics() const  { return graphics_enabled; }

private:
	friend ParsedArguments parse(int arc_int, char const** argv);

	bool graphics_enabled;

	/// The printing levels that should be enabled. Duplicate entries are possible & allowed
	std::vector<DebugLevel::Level> levels_to_enable;

	ParsedArguments(int arc_int, char const** argv);
};

/**
 * call this function to parse and return a ParsedArguments object from the
 * arguments, which then can be queried about the various options.
 */
ParsedArguments parse(int arc_int, char const** argv);

} // end namespace cmdargs

} // end namespace parsing

#endif /* PARSING__CMDARGS_PARSER_H */
