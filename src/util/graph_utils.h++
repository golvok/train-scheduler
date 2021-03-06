
#ifndef UTIL__GRAPH_UTILS_H
#define UTIL__GRAPH_UTILS_H

#include <util/iteration_utils.h++>

#include <boost/graph/astar_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <unordered_map>
#include <vector>

namespace util {

template<typename MAPPED_TO, typename GRAPH, typename... ARGS>
auto makeEdgeMap(const GRAPH& g, ARGS&&... init) {
	return std::vector<MAPPED_TO>(
		num_edges(g),
		std::forward<ARGS>(init)...
	);
}

template<typename MAPPED_TO, typename GRAPH, typename... ARGS>
auto makeVertexMap(const GRAPH& g, ARGS&&... init) {
	return std::vector<MAPPED_TO>(
		num_vertices(g),
		std::forward<ARGS>(init)...
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


template <typename K, typename V, typename MAPTYPE = std::unordered_map<K,V>>
class default_map {
public:
	using key_type = K;
	using mapped_type = V;
	using value_type = std::pair<K,V>;

	default_map(V const& defaultValue)
		: m()
		, defaultValue(defaultValue)
	{ }

	V & operator[](K const& k) {
		if (m.find(k) == m.end()) {
			m[k] = defaultValue;
		}
		return m[k];
	}

	auto begin() { return std::begin(m); }
	auto end() { return std::end(m); }
private:
	MAPTYPE m;
	V const defaultValue;
};

template <typename GRAPH, typename COST_TYPE, typename FUNC>
class astar_heuristic : public boost::astar_heuristic<GRAPH, COST_TYPE> {
private:
	FUNC f;
	using vertex_descriptor = typename boost::graph_traits<GRAPH>::vertex_descriptor;
public:
	using cost_type = COST_TYPE;

	astar_heuristic(FUNC f) : f(f) { }

	COST_TYPE operator()(const vertex_descriptor& vd) {
		return f(vd);
	}
};

template <typename GRAPH, typename FUNC>
auto make_astar_heuristic(FUNC f) {
	using vertex_descriptor = typename boost::graph_traits<GRAPH>::vertex_descriptor;
	return ::util::astar_heuristic<GRAPH,decltype(f(vertex_descriptor())),FUNC>(f);
}

template <typename KEY, typename VALUE, typename FUNC, typename MAP_TYPE = std::unordered_map<KEY,VALUE>>
class writable_function_proprety_map {
public:
	using category = boost::lvalue_property_map_tag;

	writable_function_proprety_map(const FUNC& f = FUNC()) : cache(), f(f) { }

	VALUE& operator[](const KEY& key) {
		return cache[key];
	}

	VALUE get(const KEY& k) const {
		auto find_results = cache.find(k);
		if (find_results == cache.end()) {
			return f(k);
		} else {
			return find_results->second;
		}
	}
private:
	MAP_TYPE cache;
	FUNC f;
};

template <typename KEY, typename VALUE, typename FUNC, typename MAP_TYPE>
auto put(writable_function_proprety_map<KEY,VALUE,FUNC,MAP_TYPE>& pm, const KEY& k, VALUE v) {
	pm[k] = v;
}

template <typename KEY, typename VALUE, typename FUNC, typename MAP_TYPE>
auto get(const writable_function_proprety_map<KEY,VALUE,FUNC,MAP_TYPE>& pm, const KEY& k) {
	return pm.get(k);
}

template <typename KEY, typename FUNC>
auto make_writable_function_proprety_map(FUNC f) {
	return ::util::writable_function_proprety_map<KEY,decltype(f(KEY())),FUNC>(f);
}

} // end namespace util

namespace std {

	// allow passing of writable proprety maps by std::reference_wrapper
	template<typename T, typename V, typename U>
	auto put(reference_wrapper<T> ref, V&& v, U&& u) {
		put(ref.get(),std::forward<V>(v),std::forward<U>(u));
	}
	template<typename T, typename V, typename U>
	auto get(reference_wrapper<T> ref, V&& v, U&& u) {
		get(ref.get(),std::forward<V>(v),std::forward<U>(u));
	}
}

#endif /* UTIL__GRAPH_UTILS_H */
