
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

int program_main(const std::string& data_file_name);

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

	return program_main(parsed_args.getDataFileName());
}

int program_main(const std::string& data_file_name) {

	std::list<std::thread> sim_threads;

	// these will be shared with the graphics
	std::shared_ptr<TrackNetwork> tn = std::make_shared<TrackNetwork>();
	std::shared_ptr<PassengerList> passengers = std::make_shared<PassengerList>();

	bool data_is_good;

	std::ifstream graph_in(data_file_name);

	// get the data
	std::tie(*tn,*passengers,data_is_good) = parsing::input::parse_data(graph_in);

	// if the data is bad, exit
	if (data_is_good == false) {
		return -1;
	}

	// display the track network first
	graphics::get().trainsArea().displayTrackNetwork(tn);
	// graphics::get().waitForPress();

	// then display the passengers too
	graphics::get().trainsArea().displayTNAndPassengers(tn,passengers);
	graphics::get().waitForPress();

	// do scheduling
	std::shared_ptr<algo::Schedule> schedule = std::make_shared<algo::Schedule>();
	(*schedule) = algo::schedule(*tn, *passengers);

	// display schedule
	graphics::get().trainsArea().presentResults(tn,passengers,schedule);
	graphics::get().waitForPress();

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

	sim_threads.emplace_back([](
		auto l_tn,
		auto l_passengers,
		auto l_schedule,
		auto l_p_routes,
		auto l_sim_handle
	) noexcept {

		l_sim_handle.runForTime(100, 0.3);

		auto report_engine_ptr = ::stats::make_report_engine(
			*l_tn, *l_passengers, *l_schedule, *l_p_routes, l_sim_handle
		);

		std::ofstream report_file("reports.txt");

		using ::stats::ReportConfig;
		const ReportConfig conf_prs(ReportConfig::ReportType::PASSENGER_ROUTE_STATS);
		const ReportConfig conf_sps(ReportConfig::ReportType::SIMULATION_PASSENGER_STATS);
		const ReportConfig conf_trains(ReportConfig::ReportType::TRAIN_ROUTES);

		::stats::report_into(*report_engine_ptr, conf_prs, report_file);
		::stats::report_into(*report_engine_ptr, conf_sps, report_file);
		::stats::report_into(*report_engine_ptr, conf_trains, report_file);
	},
		tn,
		passengers,
		schedule,
		p_routes,
		sim_handle
	);

	graphics::get().waitForPress();

	for (auto& thread : sim_threads) {
		thread.join();
	}

	return 0;
}
