
#ifndef STATS__REPORT_ENGINE_HPP
#define STATS__REPORT_ENGINE_HPP

#include <algo/scheduler.h++>
#include <algo/passenger_routing.h++>
#include <util/track_network.h++>
#include <stats/report_config.h++>

#include <iosfwd>

namespace stats {

class ReportEngine {
public:
	ReportEngine(
		const ::TrackNetwork& track_network,
		const std::vector<Passenger>& passengers,
		const ::algo::Schedule&	schedule,
		const ::algo::PassengerRoutes& passenger_routes
	)
		: track_network(track_network)
		, passengers(passengers)
		, schedule(schedule)
		, passenger_routes(passenger_routes)
	{ }

	void report(const ReportConfig& config, std::ostream& os);
private:
	void reportPassengerRouteStats(const ReportConfig& config, std::ostream& os);

	const ::TrackNetwork& track_network;
	const std::vector<Passenger>& passengers;
	const ::algo::Schedule&	schedule;
	const ::algo::PassengerRoutes& passenger_routes;
};

} // end namespace stats

#endif /* STATS__REPORT_ENGINE_HPP */
