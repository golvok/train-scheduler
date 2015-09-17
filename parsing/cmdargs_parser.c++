
#include "cmdargs_parser.h++"

namespace parsing {

namespace cmdargs {

ParsedArguments::ParsedArguments(int argc_int, char const** argv)
	: graphics_enabled(false)
	, levels_to_enable({DL::INFO,DL::WARN,DL::ERROR})
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
		auto debug_levels = {DL::WC_D1,DL::WC_D2,DL::WC_D3,DL::TR_D1,DL::TR_D2,DL::TR_D3,DL::PR_D1,DL::PR_D2,DL::PR_D3};
		levels_to_enable.insert(end(levels_to_enable),begin(debug_levels),end(debug_levels));
	} else {
		for (auto& arg : args) {
			if (arg.size() < 7) {
				continue;
			}
			if (arg.substr(0,6) != "--DL::") {
				continue;
			}

			auto result = DebugLevel::getFromString(arg.substr(6));
			if (result.second) {
				levels_to_enable.push_back(result.first);
			}
		}
	}

}

ParsedArguments parse(int arc_int, char const** argv) {
	return ParsedArguments(arc_int, argv);
}

} // end namespace cmdargs

} // end namespace parsing

