#include "track_network.h++"

TrackNetwork::ID TrackNetwork::getOrCreateVertex(const std::string& name) {
	const auto& find_result = name2id.find(name);
	if (find_result == name2id.end()) {
		TrackNetwork::ID id(boost::add_vertex(g()));
		name2id.emplace(name,id);
		return id;
	} else {
		return (*find_result).second;
	}
}
