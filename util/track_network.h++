#ifndef GRAPH_H
#define GRAPH_H

#include <unordered_map>
#include <boost/graph/adjacency_list.hpp>

class TrackNetwork {
	using Weight = boost::property<boost::edge_weight_t, uint>;
	using BackingGraphType = boost::adjacency_list<
		boost::vecS, boost::vecS, boost::directedS, boost::no_property, Weight
	>;

public:
	typedef boost::graph_traits<BackingGraphType>::vertex_descriptor ID;
	typedef boost::graph_traits<BackingGraphType>::edge_descriptor EdgeID;

	static const ID INVALID_ID = -1;
private:
	BackingGraphType backing_graph;
	std::unordered_map<std::string,ID> name2id;
	std::unordered_map<ID,std::pair<std::string,std::pair<float,float>>> id2data;
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

	ID createVertex(const std::string& name, std::pair<float,float> pos);
	ID getVertex(const std::string& name);
	const std::string& getVertexName(ID id);
	std::pair<float,float> getVertexPosition(ID id);

	ID getTrainSpawnLocation() { return train_spawn_location; }
	void setTrainSpawnLocation(ID id) { train_spawn_location = id; }
};

#endif /* GRAPH_H */
