
#include "report_engine.h++"

#include <ostream>
#include <stdexcept>

namespace {

} // end anonymous namespace

namespace stats {

void ReportEngine::report(const ReportConfig& config, std::ostream& os) {
	switch (config.getReportType()) {
		case ReportConfig::ReportType::PASSENGER_ROUTE_STATS:
			reportPassengerRouteStats(config,os);
			return;
	}
}

void ReportEngine::reportPassengerRouteStats(const ReportConfig& config, std::ostream& os) {
	(void)config;
	TrackNetwork::Time totalWaitingTime = 0;
	TrackNetwork::Time totalTimeInSystem = 0;

	os << "passenger, arrive time, leave time, exit time\n";
	os << "---------------------------------------------\n";

	for (const auto& passenger : passengers) {
		const auto& route = passenger_routes.getRoute(passenger);

		TrackNetwork::Time start_time = passenger.getStartTime();
		TrackNetwork::Time end_waiting_time = route.front().getTime();
		TrackNetwork::Time end_travel_time = route.back().getTime();
		totalWaitingTime += end_waiting_time - start_time;
		totalTimeInSystem += end_travel_time - end_waiting_time;

		os << passenger.getName() << ", " << start_time << ", " << end_waiting_time << ", " << end_travel_time << '\n';
	}

	os << "---------------------------------------------\n";
	os << "total waiting time   = " << totalWaitingTime << '\n';
	os << "total time in system = " << totalTimeInSystem << '\n';
}

} // end namespace stats
