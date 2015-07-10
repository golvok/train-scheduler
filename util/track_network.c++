#include "track_network.h++"

TrackNetwork::ID TrackNetwork::getOrCreateVertex(const std::string& name) {
	decltype(name2id)::iterator pos;
	bool inserted;
	std::tie(pos,inserted) = name2id.insert(std::make_pair(name, ID()) );
	if (inserted) {
		TrackNetwork::ID id(boost::add_vertex(g()));
		id2name[id] = name;
		pos->second = id;
		return id;
	} else {
		return pos->second;
	}
}

TrackNetwork::ID TrackNetwork::getVertex(const std::string& name) {
	auto find_results = name2id.find(name);
	if (find_results == name2id.end()) {
		return -1;
	} else {
		return find_results->second;
	}
}

namespace {
	std::string empty{};
}

const std::string& TrackNetwork::getNameOfVertex(TrackNetwork::ID id) {
	auto find_results = id2name.find(id);
	if (find_results == id2name.end()) {
		return empty;
	} else {
		return find_results->second;
	}
}
