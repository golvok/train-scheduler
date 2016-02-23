
#include "cmdargs_parser.h++"

namespace parsing {

namespace cmdargs {

ParsedArguments::ParsedArguments(int argc_int, char const** argv)
	: graphics_enabled(false)
	, levels_to_enable(DebugLevel::getDefaultSet())
	, singular_input_number(-1)
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

	if (std::find(begin(args),end(args),"--debug") != end(args)) {
		auto debug_levels = DebugLevel::getAllDebug();
		levels_to_enable.insert(end(levels_to_enable),begin(debug_levels),end(debug_levels));
	} else {
		std::string prefix("--DL::");
		for (auto& arg : args) {
			if (std::mismatch(begin(prefix),end(prefix),begin(arg),end(arg)).first != end(prefix)) {
				continue;
			}

			auto result = DebugLevel::getFromString(arg.substr(prefix.size(),std::string::npos));
			if (result.second) {
				auto levels_in_chain = DebugLevel::getAllShouldBeEnabled(result.first);
				levels_to_enable.insert(end(levels_to_enable),begin(levels_in_chain),end(levels_in_chain));
			}
		}
	}

	{
		auto data_flag_it = std::find(begin(args),end(args),"--data-num");
		if (data_flag_it != end(args)) {
			auto input_number_it = std::next(data_flag_it);
			if (input_number_it != end(args)) {
				singular_input_number = std::stoul(*input_number_it);
			}
		}
	}


}

ParsedArguments parse(int arc_int, char const** argv) {
	return ParsedArguments(arc_int, argv);
}

} // end namespace cmdargs

} // end namespace parsing

