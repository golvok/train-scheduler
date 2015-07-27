#include "scheduler.h++"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <iostream>

namespace {

std::vector<TrackNetwork::ID> get_shortest_route(
	TrackNetwork::ID start,
	TrackNetwork::ID end,
	TrackNetwork& network
) {
	auto& g = network.g();
	std::vector<TrackNetwork::ID> predecessors(num_vertices(g));
	std::vector<int> distances(num_vertices(g));

	dijkstra_shortest_paths(
		g, start,
		predecessor_map(
			boost::make_iterator_property_map(
				predecessors.begin(),
				get(boost::vertex_index, g)
			)
		).
		distance_map(
			boost::make_iterator_property_map(
				distances.begin(),
				get(boost::vertex_index, g)
			)
		)
	);

	std::vector<TrackNetwork::ID> route;
	dout.printIndent();
	for (
		auto vi = end;
		vi != start;
		vi = predecessors[vi]
	) {
		if (route.empty() == false && route.back() == vi) {
			dout.str() << "no route to destination! start = ";
			route.clear();
			break;
		} else {
			dout.str() << network.getNameOfVertex(vi) << " <- ";
			route.push_back(vi);
		}
	}
	dout.str() << network.getNameOfVertex(start) << '\n';
	route.push_back(start);
	util::reverse(route);

	return route;
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

} // end anonymous namespace

namespace algo {

int schedule(TrackNetwork& network, std::vector<Passenger>& passengers) {

	auto fnd_routes_indent = dout.indentWithTitle("finding routes");

	auto passenger2route = get_shortest_routes(network, passengers);
	TrackNetwork::ID spawn_loc = network.getTrainSpawnLocation();

	fnd_routes_indent.endIndent();

	auto sch_indent = dout.indentWithTitle("scheduling");

	auto sorted_passegners = passengers;
	std::sort(
		sorted_passegners.begin(),
		sorted_passegners.end(),
		[](const auto& p1, const auto& p2) {
			return p1.getStartTime() < p2.getStartTime();
		}
	);

	// ALGO:
	// if a passenger needs a train, spawn a train so that it gets there on time
	// and figure out which other passengers can be picked up, & mark them off too

	std::vector<bool> has_ride(sorted_passegners.size()); // passengers that have a ride
	std::vector<std::vector<Passenger>> train_passengers; // list of passengers on trains

	for (auto piter1 : index_assoc_iterate(sorted_passegners)) {
		Passenger& p1 = *piter1.it();
		auto& passenger_route = passenger2route.find(p1)->second;
		if (has_ride.at(piter1.i()) == true) {
			continue;
		}

		train_passengers.push_back({}); // "add" a new train
		auto pop_indent = dout.indentWithTitleF([&](auto& s){ s << "populating train " << train_passengers.size(); });
		dout << "routing to " << p1.getName() << ":\n";
		auto route_to_passenger = get_shortest_route(spawn_loc, (p1).getEntryId(), network);

		// now, iterate nodes in the paths, and find other passengers to pick up

		// people that this train will pass/pick up
		std::unordered_set<Passenger> pickupable_passengers;

		// time must be positive?
		uint time = std::max(0, (int)p1.getStartTime() - (int)route_to_passenger.size());
		dout << "train will enter at time " << time << '\n';

		// iterate over nodes in the path to this passenger, and this passenger's path
		for (auto node = route_to_passenger.begin();
			node != passenger_route.end();
			((node == route_to_passenger.end()) ? node = passenger_route.begin() : ++node), ++time
		) {
			// look at other passengers
			for (auto piter2 : index_assoc_iterate(sorted_passegners)) {
				Passenger& p2 = *piter2.it();
				if (has_ride.at(piter2.i()) == false) {
					if ((*node) == p2.getEntryId() && time >= p2.getStartTime()) {
						// this passenger enters somewhere the train will be
						pickupable_passengers.insert(p2);
					} else if (pickupable_passengers.find(p2) != pickupable_passengers.end()) {
						// this can be picked up
						if ((*node) == p2.getExitId()) {
							// this passenger can be dropped off here as well, so add it to the train
							has_ride.at(piter2.i()) = true;
							train_passengers.back().push_back(p2);
						}
					}
				}
			}
		}
		dout << "passengers are: {";
		bool first = true;
		for (const auto& p : train_passengers.back()) {
			if (!first) {
				dout.str() << ',';
			}
			first = false;
			dout.str() << p.getName();
		}
		dout.str() << "}\n";
	}

	return 0;
}

} // end namespace algo
