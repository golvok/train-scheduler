
#ifndef UTIL__ROUTING_UTILS_H
#define UTIL__ROUTING_UTILS_H

#include <util/graph_utils.h++>
#include <util/passenger.h++>
#include <util/track_network.h++>

#include <algorithm>

namespace util {

/**
 * return the shortest route according to WEIGHT_MAP
 * from p.getEntryID() to p.getExitID()
 */
template<typename WEIGHT_MAP>
std::vector<TrackNetwork::NodeID> get_shortest_route(
	const Passenger& p,
	const TrackNetwork& tn,
	const WEIGHT_MAP& weight_map
) {
	return get_shortest_route(
		p.getEntryID(),
		p.getExitID(),
		tn.g(),
		weight_map
	);
}

/**
 * return the shortest route from start to end using the default
 * weight mapping.
 */
inline std::vector<TrackNetwork::NodeID> get_shortest_route(
	TrackNetwork::NodeID start,
	TrackNetwork::NodeID end,
	const TrackNetwork& network
) {
	return get_shortest_route(
		start,
		end,
		network.g(),
		boost::get(&TrackNetwork::EdgeProperties::weight, network.g()) // default weight mapping
	);
}

/**
 * determines the shortest route for each passenger, and stores it in a map keyed by the passenger
 */
inline std::unordered_map<Passenger,typename std::vector<TrackNetwork::NodeID>> get_shortest_routes(
	TrackNetwork& network, PassengerList& passengers
) {
	std::unordered_map<Passenger,typename std::vector<TrackNetwork::NodeID>> passenger2route;

	for (auto passenger : passengers) {
		dout(DL::INFO) << "shortest path for " << passenger.getName() << " (enters at time " << passenger.getStartTime() << "):\n";

		auto route = get_shortest_route(passenger.getEntryID(),passenger.getExitID(),network);

		passenger2route.emplace(passenger,std::move(route));

	}
	return passenger2route;
}

/**
 * iterates the route, calls func(os,elem) and uses operator<< to print the result of
 * a const char[x] to os, in the format
 * vSTART -> V2 -> V3 -> V4 -> vEND
 */
template<typename CONTAINER, typename OSTREAM, typename FUNC = ::util::detail::printer>
void print_route(
	const CONTAINER& route,
	OSTREAM&& os,
	FUNC func = FUNC{}
) {
	auto beg = begin(route);
	auto en = end(route);

	os << "{ ";
	if (beg != en) {
		func(os,*beg);
		std::for_each(beg + 1, en, [&](auto& v){
			os << " -> ";
			func(os,v);
		});
	}
	os << " }";
}

/**
 * iterates the route and uses operator<< to print the result of
 * network.getVertexName(...) to os, in the format
 * vSTART -> V2 -> V3 -> V4 -> vEND
 */
template<typename CONTAINER, typename OSTREAM>
void print_route(
	const CONTAINER& route,
	const TrackNetwork& network,
	OSTREAM&& os
) {
	print_route(route, os, [&](auto&& str, auto&& v) {
		str << network.getVertexName(v);
	});
}

} // end namespace util

#endif /* UTIL__ROUTING_UTILS_H */
