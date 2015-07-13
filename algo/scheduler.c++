#include "scheduler.h++"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <iostream>

namespace algo {

int schedule(TrackNetwork& network, std::vector<Passenger>& passengers) {
	auto& g = network.g();

	for (auto passenger : passengers) {
		TrackNetwork::ID start = passenger.getEntryId();
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

		std::cout << "shortest path for " << passenger.getName() << ":\n";
		std::vector<TrackNetwork::ID> route;
		for (
			auto vi = passenger.getExitId();
			vi != passenger.getEntryId();
			vi = predecessors[vi]
		) {
			std::cout << network.getNameOfVertex(vi) << " <- ";
			route.push_back(vi);
		}
		std::cout << network.getNameOfVertex(passenger.getEntryId()) << '\n';
		route.push_back(passenger.getEntryId());

		std::reverse(route);
		passenger2route.emplace(passenger,std::move(route));

	}

	return 0;
}
	
} // end namespace algo
