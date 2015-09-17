
#include "cmdargs_parser.h++"

#include <vector>
#include <algorithm>
#include <cstdint>

namespace parsing {

namespace cmdargs {

ParsedArguments::ParsedArguments(int argc_int, char const** argv)
	: graphics_enabled(false)
 {
	uint arg_count = argc_int;
	std::vector<std::string> args;

	// add all but first
	for (uint i = 1; i < arg_count; ++i) {
		args.emplace_back(argv[i]);
	}

	if (std::find(begin(args),end(args),"--graphics") != end(args)) {
		graphics_enabled = true;
	}

}

ParsedArguments parse(int arc_int, char const** argv) {
	return ParsedArguments(arc_int, argv);
}

} // end namespace cmdargs

} // end namespace parsing

