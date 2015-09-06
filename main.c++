
#include <algo/scheduler.h++>
#include <graphics/graphics.h++>
#include <parsing/input_parser.h++>
#include <util/logging.h++>

#include <memory>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

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
		std::shared_ptr<TrackNetwork> tn = std::make_shared<TrackNetwork>();
		std::shared_ptr<std::vector<Passenger>> passengers = std::make_shared<std::vector<Passenger>>();
		bool good;

		std::tie(*tn,*passengers,good) = parsing::input::parse_data(std::cin);
		graphics::get().trainsArea().displayTrackNetwork(tn);
		graphics::get().waitForPress();

		graphics::get().trainsArea().displayTNAndPassengers(tn,passengers);
		graphics::get().waitForPress();

		if (good == false) {
			break;
		}

		dout << '\n';
		auto d_indent = dout.indentWithTitle([&](auto& s){s << "Input Data #" << tn_counter;});
		dout << '\n';

		std::shared_ptr<algo::Schedule> schedule = std::make_shared<algo::Schedule>();
		(*schedule) = algo::schedule(*tn, *passengers);

		graphics::get().trainsArea().presentResults(tn,passengers);
		graphics::get().waitForPress();
	}

	return 0;
}
