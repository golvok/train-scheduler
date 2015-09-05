
#ifndef UTIL__GRAPH_UTILS_H
#define UTIL__GRAPH_UTILS_H

#include <util/iteration_utils.h++>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>


namespace util {

template<typename MAPPED_TO, typename GRAPH, typename PARAM>
auto makeEdgeMap(const GRAPH& g, const PARAM& init) {
	return std::vector<MAPPED_TO>(
		num_edges(g),
		init
	);
}

template<typename MAPPED_TO, typename GRAPH, typename PARAM>
auto makeVertexMap(const GRAPH& g, const PARAM& init) {
	return std::vector<MAPPED_TO>(
		num_vertices(g),
		init
	);
}

template<
	typename PROP_TYPE,
	typename GRAPH,
	typename INDEX_GETTER,
	typename PROPERTY
>
auto getEdgePropertyMapCopy(
	const GRAPH& g,
	const INDEX_GETTER& pg,
	const PROPERTY& p
) {
	auto edge_prop_map_copy = makeEdgeMap<
		PROP_TYPE
	>(g,0);

	for (auto edge : make_iterable(edges(g))) {
		auto edge_index = boost::get(pg, g, edge);
		edge_prop_map_copy[edge_index] = boost::get(p, g, edge);
	}

	return edge_prop_map_copy;
}

template<typename GRAPH, typename WEIGHT_MAP>
auto get_shortest_route(
	typename boost::graph_traits<GRAPH>::vertex_descriptor start,
	typename boost::graph_traits<GRAPH>::vertex_descriptor end,
	const GRAPH& g,
	const WEIGHT_MAP& weight_map
) {
	using VertexDescriptor = typename boost::graph_traits<GRAPH>::vertex_descriptor;

	std::vector<VertexDescriptor> predecessors(num_vertices(g));

	dijkstra_shortest_paths(
		g, start,
		predecessor_map(
			boost::make_iterator_property_map(
				predecessors.begin(),
				get(boost::vertex_index, g)
			)
		).weight_map(
			weight_map
		)
	);

	std::vector<VertexDescriptor> route;
	for (
		auto vi = end;
		vi != start;
		vi = predecessors[vi]
	) {
		if (route.empty() == false && route.back() == vi) {
			route.clear();
			break;
		} else {
			route.push_back(vi);
		}
	}

	route.push_back(start);
	util::reverse(route);

	return route;
}


} // end namespace util

#endif /* UTIL__GRAPH_UTILS_H */
