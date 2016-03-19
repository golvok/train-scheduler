#include "track_network.h++"

const TrackNetwork::NodeID TrackNetwork::INVALID_ID = -1;

TrackNetwork::NodeID TrackNetwork::createVertex(const std::string& name, geom::Point<float> xy) {
	decltype(name2id)::iterator pos;
	bool inserted;
	std::tie(pos,inserted) = name2id.insert( std::make_pair(name, NodeID()) );
	if (inserted) {
		TrackNetwork::NodeID id( boost::add_vertex(g()) );
		id2data[id] = {name,xy};
		pos->second = id;
		return id;
	} else {
		return INVALID_ID;
	}
}

TrackNetwork::NodeID TrackNetwork::getVertex(const std::string& name) const {
	auto find_results = name2id.find(name);
	if (find_results == name2id.end()) {
		return INVALID_ID;
	} else {
		return find_results->second;
	}
}

namespace {
	const std::string empty{};
}

const std::string& TrackNetwork::getVertexName(TrackNetwork::NodeID id) const {
	auto find_results = id2data.find(id);
	if (find_results == id2data.end()) {
		return empty;
	} else {
		return find_results->second.first;
	}
}

geom::Point<float> TrackNetwork::getVertexPosition(TrackNetwork::NodeID id) const {
	auto find_results = id2data.find(id);
	if (find_results == id2data.end()) {
		return geom::Point<float>();
	} else {
		return find_results->second.second;
	}
}

TrackNetwork::EdgeIndex TrackNetwork::getEdgeIndex(EdgeID eid) const {
	return boost::get(&TrackNetwork::EdgeProperties::index, g(), eid);
}

std::vector<TrackNetwork::Weight> TrackNetwork::makeEdgeWeightMapCopy() const {
	return ::util::getEdgePropertyMapCopy<Weight>(g(),&EdgeProperties::index, &EdgeProperties::weight);
}

TrackNetwork::Weight TrackNetwork::getDistanceBetween(std::pair<NodeID,NodeID> edge) const {
	return boost::get(
		&TrackNetwork::EdgeProperties::weight,
		g(),
		boost::edge(edge.first, edge.second, g()).first
	);
}
