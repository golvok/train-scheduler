#include "scheduler.h++"

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <iostream>

namespace algo {

int schedule(TrackNetwork& network, std::vector<Train>& trains) {
	auto& g = network.g();

	for (auto train : trains) {
		TrackNetwork::ID start = train.getEntryId();
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

		std::cout << "distances and parents: for " << train.getName() << '\n';
		auto verts = vertices(g);
		for (auto vi = verts.first; vi != verts.second; ++vi) {
		  std::cout << "distance(" << network.getNameOfVertex(*vi) << ") = " << distances[*vi] << ", ";
		  std::cout << "parent(" << network.getNameOfVertex(*vi) << ") = " << network.getNameOfVertex(predecessors[*vi]) << std::
		    endl;
		}
		std::cout << "\n\n";
	}

	return 0;
}
	
} // end namespace algo
