
#include <algo/scheduler.h++>
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
	dout.setMaxIndentation(7);

	auto parsed_args = parsing::cmdargs::parse(argc,argv);

	if (parsed_args.shouldEnableGraphics()) {
		graphics::get().initialize();
	}

	for (std::underlying_type_t<DebugLevel::Level> i = 0; i < DL::LEVEL_COUNT; ++i) {
		dout.enable_level(static_cast<DebugLevel::Level>(i));
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

		dout(DL::INFO) << '\n';
		auto d_indent = dout(DL::INFO).indentWithTitle([&](auto&& s){s << "Input Data #" << tn_counter;});
		dout(DL::INFO) << '\n';

		std::shared_ptr<algo::Schedule> schedule = std::make_shared<algo::Schedule>();
		(*schedule) = algo::schedule(*tn, *passengers);

		graphics::get().trainsArea().presentResults(tn,passengers,schedule);
		graphics::get().waitForPress();
	}

	return 0;
}
