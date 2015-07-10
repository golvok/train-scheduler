#ifndef GRAPH_H
#define GRAPH_H

#include <unordered_map>
#include <boost/graph/adjacency_list.hpp>

class TrackNetwork {
	using Weight = boost::property<boost::edge_weight_t, uint>;
	using BackingGraphType = boost::adjacency_list<
		boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, Weight
	>;

public:
	typedef boost::graph_traits<BackingGraphType>::vertex_descriptor ID;
	typedef boost::graph_traits<BackingGraphType>::edge_descriptor EdgeID;

private:
	BackingGraphType backing_graph;
	std::unordered_map<std::string,ID> name2id;
	std::unordered_map<ID,std::string> id2name;

public:
	TrackNetwork()
		: backing_graph()
		, name2id()
		, id2name()
	{}

	TrackNetwork& operator=(const TrackNetwork&) = default;
	TrackNetwork(const TrackNetwork&) = default;
	TrackNetwork(TrackNetwork&&) = default;

	BackingGraphType& g() { return backing_graph; }

	ID getOrCreateVertex(const std::string& name);
	ID getVertex(const std::string& name);
	const std::string& getNameOfVertex(ID id);
};

#endif /* GRAPH_H */
