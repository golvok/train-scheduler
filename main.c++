
#include <algo/scheduler.h++>
#include <algo/passenger_routing.h++>
#include <graphics/graphics.h++>
#include <parsing/input_parser.h++>
#include <parsing/cmdargs_parser.h++>
#include <stats/report_config.h++>
#include <stats/report_engine.h++>
#include <util/logging.h++>

#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
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
		std::shared_ptr<PassengerList> passengers = std::make_shared<PassengerList>();

		bool data_is_good;

		// get the data
		std::tie(*tn,*passengers,data_is_good) = parsing::input::parse_data(std::cin);

		// if the data is bad, exit
		if (data_is_good == false) {
			break;
		}

		// display the track network first
		graphics::get().trainsArea().displayTrackNetwork(tn);
		// graphics::get().waitForPress();

		// then display the passengers too
		graphics::get().trainsArea().displayTNAndPassengers(tn,passengers);
		// graphics::get().waitForPress();

		dout(DL::INFO) << '\n';
		auto d_indent = dout(DL::INFO).indentWithTitle([&](auto&& s){s << "Input Data #" << tn_counter;});
		dout(DL::INFO) << '\n';

		// do scheduling
		std::shared_ptr<algo::Schedule> schedule = std::make_shared<algo::Schedule>();
		(*schedule) = algo::schedule(*tn, *passengers);

		// display schedule
		graphics::get().trainsArea().presentResults(tn,passengers,schedule);
		// graphics::get().waitForPress();

		// route passengers
		auto p_routes = std::make_shared<::algo::PassengerRoutes>();
		*p_routes = ::algo::route_passengers(*tn,*schedule,*passengers);

		auto sim_handle = ::sim::instantiate_simulator(
			passengers,
			schedule,
			tn
		);

		// display a simulation
		graphics::get().trainsArea().displaySimulator(sim_handle);

		std::thread sim_thread([&]() {
			sim_handle.runForTime(20, 0.3);
		});
		sim_thread.detach();

		graphics::get().waitForPress();

		::stats::ReportEngine report_engine(*tn,*passengers,*schedule,*p_routes);

		::stats::ReportConfig conf(::stats::ReportConfig::ReportType::PASSENGER_ROUTE_STATS);
		std::ofstream report_file("reports.txt");
		report_engine.report(conf,report_file);
	}

	return 0;
}
