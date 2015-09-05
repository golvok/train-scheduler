#include "scheduler.h++"

#include <util/graph_utils.h++>
#include <util/iteration_utils.h++>
#include <util/logging.h++>
#include <util/routing_utils.h++>

#include <boost/property_map/function_property_map.hpp>
#include <iostream>
#include <unordered_set>

namespace algo {

class Scheduler {

	const TrackNetwork& network;
	const TrackNetwork::BackingGraphType& g;
	const std::vector<Passenger>& passengers;

public:
	Scheduler(
		const TrackNetwork& network,
		const std::vector<Passenger>& passengers
	)
	: network(network)
	, g(network.g())
	, passengers(passengers)
	{ }

	/**
	 * Main entry-point flow function. Calls all the other ones.
	 */
	Schedule do_schedule();
};

/**
 * Entry Point.
 *
 * Calls flow function
 */
Schedule schedule(
	const TrackNetwork& network,
	const std::vector<Passenger>& passengers
) {
	return Scheduler (
		network,
		passengers
	).do_schedule();
}

Schedule Scheduler::do_schedule() {
	return Schedule("abc");

}

} // end namespace algo
