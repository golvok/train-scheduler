#include "track_network.h++"

#include <cassert>

TrackNetwork::ID TrackNetwork::createVertex(const std::string& name, geom::Point<float> xy) {
	decltype(name2id)::iterator pos;
	bool inserted;
	std::tie(pos,inserted) = name2id.insert( std::make_pair(name, ID()) );
	if (inserted) {
		TrackNetwork::ID id( boost::add_vertex(g()) );
		id2data[id] = {name,xy};
		pos->second = id;
		return id;
	} else {
		return INVALID_ID;
	}
}

TrackNetwork::ID TrackNetwork::getVertex(const std::string& name) const {
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

const std::string& TrackNetwork::getVertexName(TrackNetwork::ID id) const {
	auto find_results = id2data.find(id);
	if (find_results == id2data.end()) {
		return empty;
	} else {
		return find_results->second.first;
	}
}

geom::Point<float> TrackNetwork::getVertexPosition(TrackNetwork::ID id) const {
	auto find_results = id2data.find(id);
	if (find_results == id2data.end()) {
		return geom::Point<float>();
	} else {
		return find_results->second.second;
	}
}
