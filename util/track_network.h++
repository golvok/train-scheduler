#ifndef GRAPH_H
#define GRAPH_H

#include <graphics/geometry.h++>
#include <util/utils.h++>

#include <boost/graph/adjacency_list.hpp>
#include <unordered_map>
#include <limits>
#include <vector>

#include <util/graph_utils.h++> // needs to be after the graph definition

struct StationIdTag {
	static const uint DEFAULT_VALUE = -1;
};
using StationId = ::util::ID<uint, StationIdTag>;

class TrackNetwork {
public:
	using Time = uint;

	using Weight = float;
	using EdgeIndex = uint;
	struct EdgeProperties {
		Weight weight;
		EdgeIndex index;
		EdgeProperties()
			: weight(std::numeric_limits<decltype(weight)>::max())
			, index(-1)
		{ }
		EdgeProperties(Weight w, EdgeIndex ei)
			: weight(w)
			, index(ei)
		{ }
	};

	using BackingGraphType = boost::adjacency_list<
		boost::vecS, boost::vecS, boost::directedS, boost::no_property, EdgeProperties
	>;

	using ID        = boost::graph_traits<BackingGraphType>::vertex_descriptor;
	using EdgeID    = boost::graph_traits<BackingGraphType>::edge_descriptor;

	using EdgeWeightMap = std::vector<Weight>;

	static const ID INVALID_ID = -1;
private:
	BackingGraphType backing_graph;
	std::unordered_map<std::string,ID> name2id;
	std::unordered_map<ID,std::pair<std::string,geom::Point<float>>> id2data;
	ID train_spawn_location;

public:
	TrackNetwork()
		: backing_graph()
		, name2id()
		, id2data()
		, train_spawn_location()
	{}

	TrackNetwork& operator=(const TrackNetwork&) = default;
	TrackNetwork(const TrackNetwork&) = default;
	TrackNetwork(TrackNetwork&&) = default;

	BackingGraphType& g() { return backing_graph; }
	const BackingGraphType& g() const { return backing_graph; }

	ID createVertex(const std::string& name, geom::Point<float> pos);
	ID getVertex(const std::string& name) const;
	const std::string& getVertexName(ID id) const;
	geom::Point<float> getVertexPosition(ID id) const;

	ID getTrainSpawnLocation() const { return train_spawn_location; }
	void setTrainSpawnLocation(ID id) { train_spawn_location = id; }

	EdgeIndex getEdgeIndex(EdgeID eid) const;
	EdgeWeightMap makeEdgeWeightMapCopy() const;

	StationId getStationIdByVertexId(ID id) const { return ::util::make_id<StationId>(id); }
	ID getVertexIdByStationId(StationId sid) const { return sid.getValue(); }

	template<typename MAPPED_TYPE, typename... ARGS>
	auto makeStationMap(ARGS&&... args) {
		// maybe add a wrapper that forces operator [] to only accept StationId
		// inherit from vector, use templae vararg function to call base?
		return ::util::makeVertexMap<MAPPED_TYPE>(g(), std::forward<ARGS>(args)...);
	}

};

template<typename MAPPED_TYPE>
using StationMap = decltype(TrackNetwork().makeStationMap<MAPPED_TYPE>());

#endif /* GRAPH_H */
