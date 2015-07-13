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

	uint tn_counter = 0;

	while (true) {
		tn_counter += 1;
		TrackNetwork tn;
		std::vector<Passenger> passengers;
		bool good;

		std::tie(tn,passengers,good) = parsing::input::parse_graph(std::cin, std::cerr);

		if (good == false) {
			break;
		}

		std::cout << "\nInput Data #" << tn_counter << ":\n";

		auto results = algo::schedule(tn, passengers);
		(void)results;

	}

	return 0;
}
