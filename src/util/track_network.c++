#include "track_network.h++"

#include <util/utils.h++>

const TrackNetwork::NodeID TrackNetwork::INVALID_NODE_ID = -1;
const TrackNetwork::Time TrackNetwork::INVALID_TIME = std::numeric_limits<TrackNetwork::Time>::min();

TrackNetwork::TrackNetwork(
	BackingGraphType&& network,
	OffNodeDataPropertyMap&& offNodeData
)
	: backing_graph(std::move(network))
	, name2id()
	, id2data(offNodeData)
	, train_spawn_location()
{
	for (const auto& vdesc : make_iterable(vertices(backing_graph))) {
		const auto& off_node_data = get(id2data, vdesc);
		name2id.emplace(off_node_data.name, vdesc);
	}
}

TrackNetwork::NodeID TrackNetwork::getVertex(const std::string& name) const {
	auto find_results = name2id.find(name);
	if (find_results == name2id.end()) {
		return INVALID_NODE_ID;
	} else {
		return find_results->second;
	}
}

namespace {
	const std::string empty{};
}

const std::string& TrackNetwork::getVertexName(TrackNetwork::NodeID id) const {
	return get(id2data, id).name;
}

TrackNetwork::PointType TrackNetwork::getVertexPosition(TrackNetwork::NodeID id) const {
	return get(id2data, id).location;
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
