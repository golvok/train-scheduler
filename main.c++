
#include <algo/scheduler.h++>
#include <algo/passenger_routing.h++>
#include <graphics/graphics.h++>
#include <parsing/input_parser.h++>
#include <parsing/cmdargs_parser.h++>
#include <util/logging.h++>

#include <memory>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

int program_main();

int main(int argc, char const** argv) {
	dout.setHighestTitleRank(7);

	auto parsed_args = parsing::cmdargs::parse(argc,argv);

	// enable graphics
	if (parsed_args.shouldEnableGraphics()) {
		graphics::get().initialize();
	}

	// enable logging levels
	for (auto& l : parsed_args.getDebugLevelsToEnable()) {
		dout.enable_level(l);
	}

	return program_main();
}

int program_main() {
	uint tn_counter = 0;

	while (true) {
		tn_counter += 1;

		// these will be shared with the graphics
		std::shared_ptr<TrackNetwork> tn = std::make_shared<TrackNetwork>();
		std::shared_ptr<std::vector<Passenger>> passengers = std::make_shared<std::vector<Passenger>>();

		bool data_is_good;

		// get the data
		std::tie(*tn,*passengers,data_is_good) = parsing::input::parse_data(std::cin);

		// if the data is bad, exit
		if (data_is_good == false) {
			break;
		}

		// display the track network first
		graphics::get().trainsArea().displayTrackNetwork(tn);
		graphics::get().waitForPress();

		// then display the passengers too
		graphics::get().trainsArea().displayTNAndPassengers(tn,passengers);
		graphics::get().waitForPress();

		dout(DL::INFO) << '\n';
		auto d_indent = dout(DL::INFO).indentWithTitle([&](auto&& s){s << "Input Data #" << tn_counter;});
		dout(DL::INFO) << '\n';

		// do scheduling
		std::shared_ptr<algo::Schedule> schedule = std::make_shared<algo::Schedule>();
		(*schedule) = algo::schedule(*tn, *passengers);

		// display schedule
		graphics::get().trainsArea().presentResults(tn,passengers,schedule);
		graphics::get().waitForPress();

		// route passengers
		::algo::route_passengers(*tn,*schedule,*passengers);

		graphics::get().waitForPress();
	}

	return 0;
}
