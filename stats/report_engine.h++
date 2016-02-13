
#ifndef STATS__REPORT_ENGINE_HPP
#define STATS__REPORT_ENGINE_HPP

#include <sim/simulator.h++>

#include <algo/scheduler.h++>
#include <algo/passenger_routing.h++>
#include <util/track_network.h++>
#include <stats/report_config.h++>

#include <iosfwd>
#include <memory>

namespace stats {

class ReportEngine;

class ReportEngineHandle {
	std::unique_ptr<ReportEngine> ptr;
public:
	ReportEngineHandle(std::unique_ptr<ReportEngine>&& ptr);
	ReportEngineHandle(ReportEngineHandle&&);
	~ReportEngineHandle();
	ReportEngine& operator*();
	ReportEngine* operator->();
	const ReportEngine& operator*() const;
};

ReportEngineHandle make_report_engine(
	const ::TrackNetwork& track_network,
	const PassengerList& passengers,
	const ::algo::Schedule&	schedule,
	const ::algo::PassengerRoutes& passenger_routes,
	const ::sim::SimulatorHandle& sim_handle
);

void report_into(
	ReportEngine& rengine,
	const ReportConfig& config,
	std::ostream& os
);

} // end namespace stats

#endif /* STATS__REPORT_ENGINE_HPP */
