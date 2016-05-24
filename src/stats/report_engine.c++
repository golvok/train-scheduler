
#include "report_engine_internal.h++"

#include <sim/simulator_internal.h++>
#include <util/routing_utils.h++>

#include <ostream>
#include <stdexcept>

namespace {

} // end anonymous namespace

namespace stats {

ReportEngineHandle::~ReportEngineHandle() { }
ReportEngineHandle::ReportEngineHandle(std::unique_ptr<ReportEngine>&& ptr) : ptr(std::move(ptr)) { }
ReportEngineHandle::ReportEngineHandle(ReportEngineHandle&& src) : ptr(std::move(src.ptr)) { }
ReportEngine& ReportEngineHandle::operator*() { return *ptr; }
ReportEngine* ReportEngineHandle::operator->() { return ptr.get(); }
const ReportEngine& ReportEngineHandle::operator*() const { return *ptr; }


ReportEngineHandle make_report_engine(
	const ::TrackNetwork& track_network,
	const PassengerList& passengers,
	const ::algo::Schedule&	schedule,
	const ::algo::PassengerRoutes& passenger_routes,
	const ::sim::SimulatorHandle& sim_handle
) {
	return ReportEngineHandle(std::make_unique<ReportEngine>(
		track_network,
		passengers,
		schedule,
		passenger_routes,
		sim_handle
	));
}

void report_into(
	ReportEngine& rengine,
	const ReportConfig& config,
	std::ostream& os
) {
	switch (config.getReportType()) {
		case ReportConfig::ReportType::PASSENGER_ROUTE_STATS:
			rengine.reportPassengerRouteStats(config,os);
			return;
		case ReportConfig::ReportType::SIMULATION_PASSENGER_STATS:
			rengine.reportSimulationPassengerStats(config,os);
			return;
		case ReportConfig::ReportType::TRAIN_ROUTES:
			rengine.reportTrains(config,os);
			return;
	}
}

void ReportEngine::reportPassengerRouteStats(const ReportConfig& config, std::ostream& os) {
	(void)config;
	TrackNetwork::Time totalWaitingTime = 0;
	TrackNetwork::Time totalTimeInSystem = 0;

	os << "Passenger Route Statistics Report\n";
	os << "passenger, arrive time, departure time, arrival time\n";
	os << "---------------------------------------------\n";

	for (const auto& passenger : passengers) {
		const auto& route = passenger_routes.getRoute(passenger);

		TrackNetwork::Time start_time = passenger.getStartTime();
		TrackNetwork::Time end_waiting_time = std::next(route.begin())->getTime();
		TrackNetwork::Time end_travel_time = route.back().getTime();
		totalWaitingTime += end_waiting_time - start_time;
		totalTimeInSystem += end_travel_time - end_waiting_time;

		os << passenger.getName() << ", " << start_time << ", " << end_waiting_time << ", " << end_travel_time;
		os << " : Path = ";
		::util::print_route_of_route_elements(route, track_network, os);
		os << '\n';
	}

	os << "---------------------------------------------\n";
	os << "total waiting time   = " << totalWaitingTime << '\n';
	os << "total time on trains = " << totalTimeInSystem << '\n';
	os << "\n\n\n";
}

void ReportEngine::reportSimulationPassengerStats(const ReportConfig& config, std::ostream& os) {
	(void)config;
	const auto& passenger_exits = sim_handle.get()->getPassengerExits();

	::sim::SimTime totalWaitingTime = 0;
	::sim::SimTime totalTimeInSystem = 0;

	os << "Simulation Passenger Statistics Report\n";
	os << "passenger, arrive time, departure time, arrival time\n";
	os << "---------------------------------------------\n";

	for (const auto& passenger : passengers) {
		const auto& route = passenger_routes.getRoute(passenger);
		const auto exit_it = std::find_if(passenger_exits.begin(), passenger_exits.end(),
			[&](const auto& elem) {
				return elem.passenger.get() == passenger;
			}
		);

		::sim::SimTime start_time = passenger.getStartTime();
		::sim::SimTime end_waiting_time = std::next(route.begin())->getTime();
		totalWaitingTime += end_waiting_time - start_time;

		os << passenger.getName() << ", " << start_time << ", " << end_waiting_time << ", ";

		if (exit_it == passenger_exits.end()) {
			os << "--";
		} else {
			totalTimeInSystem += exit_it->time_of_exit - end_waiting_time;
			os << exit_it->time_of_exit;
		}

		os << '\n';
	}

	os << "---------------------------------------------\n";
	os << "total waiting time   = " << totalWaitingTime << '\n';
	os << "total time on trains = " << totalTimeInSystem << '\n';
	os << "\n\n\n";
}

void ReportEngine::reportTrains(const ReportConfig& config, std::ostream& os) {
	(void)config;

	os << "Report of Trains & Their Routes\n";
	os << "---------------------------------------------\n";

	for (const auto& route : schedule.getTrainRoutes()) {
		os << route << '\n';
	}

	os << "---------------------------------------------\n";
	os << "\n\n\n";
}

} // end namespace stats
