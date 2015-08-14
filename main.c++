
#include <algo/scheduler.h++>
#include <graphics/graphics.h++>
#include <parsing/input_parser.h++>
#include <util/logging.h++>
#include <util/utils.h++>

#include <string>
#include <vector>
#include <iostream>

int program_main();

int main(int argc_int, char const** argv) {
	dout.setMaxIndentation(7);

	uint arg_count = argc_int;
	std::vector<std::string> args;
	for (uint i = 0; i < arg_count; ++i) {
		args.emplace_back(argv[i]);
	}

	if (std::find(args.begin(),args.end(),"--graphics") != args.end()) {
		graphics::get().initialize();
	}

	return program_main();
}

int program_main() {
	uint tn_counter = 0;

	while (true) {
		tn_counter += 1;
		TrackNetwork tn;
		std::vector<Passenger> passengers;
		bool good;

		std::tie(tn,passengers,good) = parsing::input::parse_data(std::cin);
		graphics::get().getTrainsAreaData().setTN(tn);

		if (good == false) {
			break;
		}

		dout << '\n';
		auto d_indent = dout.indentWithTitle([&](auto& s){s << "Input Data #" << tn_counter;});
		dout << '\n';

		auto results = algo::schedule(tn, passengers);
		(void)results;

		graphics::get().waitForPress();
	}

	return 0;
}
