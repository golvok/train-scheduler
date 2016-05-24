#ifndef GRAPH_H
#define GRAPH_H

#include <graphics/geometry.h++>
#include <util/generator.h++>

#include <boost/graph/adjacency_list.hpp>
#include <boost/range/iterator_range.hpp>
#include <unordered_map>
#include <limits>
#include <vector>

#include <util/graph_utils.h++> // needs to be after the graph definition

struct StationIDTag {
	static const uint DEFAULT_VALUE = -1;
};
using StationID = ::util::ID<uint, StationIDTag>;

class TrackNetwork {
public:
	using Time = int;
	using TimeInterval = std::pair<Time,Time>;

	static const Time INVALID_TIME;

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

	using NodeID    = boost::graph_traits<BackingGraphType>::vertex_descriptor;
	using EdgeID    = boost::graph_traits<BackingGraphType>::edge_descriptor;

	using EdgeWeightMap = std::vector<Weight>;

	static const NodeID INVALID_NODE_ID;
private:
	BackingGraphType backing_graph;
	std::unordered_map<std::string,NodeID> name2id;
	std::unordered_map<NodeID,std::pair<std::string,geom::Point<float>>> id2data;
	NodeID train_spawn_location;

public:
	TrackNetwork()
		: backing_graph()
		, name2id()
		, id2data()
		, train_spawn_location()
	{}

	TrackNetwork& operator=(const TrackNetwork&) = delete;
	TrackNetwork(const TrackNetwork&) = delete;
	TrackNetwork& operator=(TrackNetwork&&) = default;
	TrackNetwork(TrackNetwork&&) = default;

	BackingGraphType& g() { return backing_graph; }
	const BackingGraphType& g() const { return backing_graph; }

	NodeID createVertex(const std::string& name, geom::Point<float> pos);
	NodeID getVertex(const std::string& name) const;
	const std::string& getVertexName(NodeID id) const;
	geom::Point<float> getVertexPosition(NodeID id) const;

	NodeID getTrainSpawnLocation() const { return train_spawn_location; }
	void setTrainSpawnLocation(NodeID id) { train_spawn_location = id; }

	EdgeIndex getEdgeIndex(EdgeID eid) const;
	EdgeWeightMap makeEdgeWeightMapCopy() const;

	StationID getStationIDByVertexID(NodeID id) const { return ::util::make_id<StationID>(id); }
	NodeID getVertexIDByStationID(StationID sid) const { return sid.getValue(); }

	auto getStaitonRange() const {
		return ::util::make_generator<decltype(vertices(g()).first)>(
			vertices(g()).first,
			vertices(g()).second,
			[&](const auto& it) {
				return std::next(it);
			},
			[&](const auto& it) {
				return this->getStationIDByVertexID(*it);
			}
		);
	}

	template<typename MAPPED_TYPE, typename... ARGS>
	auto makeStationMap(ARGS&&... args) const {
		// maybe add a wrapper that forces operator [] to only accept StationID
		// inherit from vector, use templae vararg function to call base?
		// inheriting is probably a bad idea... anything that expects a vector
		// will upcast and nullify restrictions... wait... is that fine?
		return ::util::makeVertexMap<MAPPED_TYPE>(g(), std::forward<ARGS>(args)...);
	}

	Weight getDistanceBetween(std::pair<NodeID,NodeID> edge) const;

	template<typename ITER, typename SPEED_FUNC>
	Time sumTimeTakenWithCustomSpeed(::boost::iterator_range<ITER> range, SPEED_FUNC sf) const;

};

template<typename MAPPED_TYPE>
using StationMap = decltype(TrackNetwork().makeStationMap<MAPPED_TYPE>());

//// BEGIN ILINE & TEMPLATE FUNCTIONS ////

template<typename ITER, typename SPEED_FUNC>
TrackNetwork::Time TrackNetwork::sumTimeTakenWithCustomSpeed(::boost::iterator_range<ITER> range, SPEED_FUNC sf) const {
	TrackNetwork::Time result = 0;
	auto current_edge = std::make_pair(TrackNetwork::INVALID_NODE_ID,TrackNetwork::INVALID_NODE_ID);
	for (const auto& vid : range) {
		current_edge.first = current_edge.second;
		current_edge.second = vid;
		if (current_edge.first != TrackNetwork::INVALID_NODE_ID) {
			result += this->getDistanceBetween(
				current_edge
			) / (
				sf(current_edge)
			);
		}
	}
	return result;
}


#endif /* GRAPH_H */
