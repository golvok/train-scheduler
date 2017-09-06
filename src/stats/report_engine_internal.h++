
#include "report_engine.h++"

#include <stats/report_config.h++>

namespace stats {

class ReportEngine {
public:
	ReportEngine(
		const ::TrackNetwork& track_network,
		const ::algo::Schedule&	schedule,
		const ::sim::SimulatorHandle& sim_handle
	)
		: track_network(track_network)
		, schedule(schedule)
		, sim_handle(sim_handle)
	{ }

private:
	void reportPassengerRouteStats(const ReportConfig& config, std::ostream& os);
	void reportSimulationPassengerStats(const ReportConfig& config, std::ostream& os);
	void reportTrains(const ReportConfig& config, std::ostream& os);

	const ::TrackNetwork& track_network;
	const ::algo::Schedule&	schedule;
	const ::sim::SimulatorHandle& sim_handle;

	const auto& getPassengers() { return sim_handle.getPassengerList(); }

	friend void report_into(
		ReportEngine& rengine,
		const ReportConfig& config,
		std::ostream& os
	);
};

} // end namespace stats
