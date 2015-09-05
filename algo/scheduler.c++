#include "scheduler.h++"

#include <util/graph_utils.h++>
#include <util/iteration_utils.h++>
#include <util/logging.h++>
#include <util/routing_utils.h++>

#include <boost/property_map/function_property_map.hpp>
#include <iostream>
#include <unordered_set>

namespace algo {

Schedule schedule(
	const TrackNetwork& network,
	const std::vector<Passenger>& passengers
) {
	(void)network;
	(void)passengers;

	return Schedule("test");
}

} // end namespace algo
