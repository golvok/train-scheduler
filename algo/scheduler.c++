#include "scheduler.h++"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <iostream>

namespace algo {

int schedule(TrackNetwork& network, std::vector<Passenger>& passengers) {
	auto& g = network.g();

	for (auto person : passengers) {
		TrackNetwork::ID start = person.getEntryId();
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

		std::cout << "distances and parents: for " << person.getName() << '\n';
		for (auto vi : make_iterable(vertices(g))) {
			std::cout
				<< "distance(" << network.getNameOfVertex(vi) << ") = "
					<< distances[vi] << ", "
				<< "parent(" << network.getNameOfVertex(vi) << ") = "
					<< network.getNameOfVertex(predecessors[vi])
				<< std::endl
		    ;
		}
		std::cout << "\n\n";
	}

	return 0;
}
	
} // end namespace algo
