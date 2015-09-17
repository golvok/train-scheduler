
#ifndef PARSING__CMDARGS_PARSER_H
#define PARSING__CMDARGS_PARSER_H

#include <util/logging.h++>

namespace parsing {

namespace cmdargs {

class ParsedArguments {
public:
	const std::vector<DebugLevel::Level> getDebugLevelsToEnable() {
		return levels_to_enable;
	}

	bool shouldEnableGraphics() { return graphics_enabled; }

private:
	friend ParsedArguments parse(int arc_int, char const** argv);

	bool graphics_enabled;

	/// The printing levels that should be enabled. Duplicate entries are fine.
	std::vector<DebugLevel::Level> levels_to_enable;

	ParsedArguments(int arc_int, char const** argv);
};

ParsedArguments parse(int arc_int, char const** argv);

} // end namespace cmdargs

} // end namespace parsing

#endif /* PARSING__CMDARGS_PARSER_H */
