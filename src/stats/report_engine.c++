
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
	const ::algo::Schedule&	schedule,
	const ::sim::SimulatorHandle& sim_handle
) {
	return ReportEngineHandle(std::make_unique<ReportEngine>(
		track_network,
		schedule,
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
	os << "passenger, start time, departure time, arrival time, path...\n";
	os << "---------------------------------------------\n";

	for (const auto& value_pair : getPassengers()) {
		const auto& passenger = value_pair.second;
		const auto& [route, ch] = algo::route_through_schedule(
			track_network,
			schedule,
			passenger.getStartTime(),
			passenger.getEntryID(),
			passenger.getExitID()
		);
		(void)ch;

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
	const auto& passenger_histories = sim_handle.get()->getPassengerHistories();

	::sim::SimTime totalWaitingTime = 0;
	::sim::SimTime totalTimeInSystem = 0;

	os << "Simulation Passenger Statistics Report\n";
	os << "passenger, start time, departure time, arrival time\n";
	os << "---------------------------------------------\n";

	for (const auto& value_pair : getPassengers()) {
		const auto& passenger = value_pair.second;
		const auto& hist_find_result = passenger_histories.find(passenger);
		if (hist_find_result == end(passenger_histories)) {
			continue;
		}
		const auto& psgr_history = hist_find_result->second;
		if (psgr_history.getRoute().empty()) {
			continue;
		}

		::sim::SimTime start_time = passenger.getStartTime();
		::sim::SimTime end_waiting_time = std::next(psgr_history.getRoute().begin())->getTime();
		totalWaitingTime += end_waiting_time - start_time;

		os << passenger.getName() << ", " << start_time << ", " << end_waiting_time << ", ";

		totalTimeInSystem += psgr_history.time_of_exit - end_waiting_time;
		os << psgr_history.time_of_exit;

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
		os << std::tie(route, track_network) << '\n';
	}

	os << "---------------------------------------------\n";
	os << "\n\n\n";
}

} // end namespace stats
