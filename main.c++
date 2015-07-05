#include <util/utils.h++>
#include <parsing/input_parser.h++>
#include <algo/scheduler.h++>

#include <string>
#include <vector>
#include <iostream>

using uint = unsigned int;

int main(int argc_int, char const** arcv) {
	uint arg_count = argc_int;
	std::vector<std::string> args;
	for (uint i = 0; i < arg_count; ++i) {
		args.emplace_back(arcv[i]);
	}
	(void)args;

	while (true) {
		Graph::TrackGraph g;
		std::vector<Train> trains;
		bool good;

		std::tie(g,trains,good) = parsing::input::parse_graph(std::cin, std::cerr);

		if (good == false) {
			break;
		}

		auto results = algo::schedule(g, trains);
		(void)results;

	}

	return 0;
}
