
#ifndef UTIL__ROUTING_UTILS_H
#define UTIL__ROUTING_UTILS_H

#include <util/graph_utils.h++>
#include <util/track_network.h++>

#include <algorithm>

namespace util {

template<typename WEIGHT_MAP>
std::vector<TrackNetwork::ID> get_shortest_route(
	const Passenger& p,
	const TrackNetwork& tn,
	const WEIGHT_MAP& weight_map
) {
	return get_shortest_route(
		p.getEntryId(),
		p.getExitId(),
		tn.g(),
		weight_map
	);
}

std::vector<TrackNetwork::ID> get_shortest_route(
	TrackNetwork::ID start,
	TrackNetwork::ID end,
	const TrackNetwork& network
) {
	return get_shortest_route(
		start,
		end,
		network.g(),
		boost::get(&TrackNetwork::EdgeProperties::weight, network.g()) // default weight mapping
	);
}

std::unordered_map<Passenger,typename std::vector<TrackNetwork::ID>> get_shortest_routes(
	TrackNetwork& network, std::vector<Passenger>& passengers
) {
	std::unordered_map<Passenger,typename std::vector<TrackNetwork::ID>> passenger2route;

	for (auto passenger : passengers) {
		dout << "shortest path for " << passenger.getName() << " (enters at time " << passenger.getStartTime() << "):\n";

		auto route = get_shortest_route(passenger.getEntryId(),passenger.getExitId(),network);

		passenger2route.emplace(passenger,std::move(route));

	}
	return passenger2route;
}

template<typename CONTAINER, typename OSTREAM>
void print_route(
	const CONTAINER& route,
	const TrackNetwork& network,
	OSTREAM& os
) {
	std::for_each(std::begin(route), std::end(route), [&](auto& v){
		os << network.getVertexName(v) << " -> ";
	});
	os << "|\n";
}

} // end namespace util

#endif /* UTIL__ROUTING_UTILS_H */
