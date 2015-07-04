#ifndef GRAPH_H
#define GRAPH_H

#include <unordered_map>
#include <boost/graph/adjacency_list.hpp>

namespace Graph {

class TrackGraph {
public:
	using Weight = boost::property<boost::edge_weight_t, uint>;
	using BackingGraphType = boost::adjacency_list<
		boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, Weight
	>;

	class ID {
		typedef boost::graph_traits<BackingGraphType>::vertex_descriptor value_type;
		value_type value;
		ID(value_type value) : value(value) { }
		value_type get_value() const { return value; }

		friend class TrackGraph;
		friend struct std::hash<ID>;
	};

private:
	BackingGraphType backing_graph;
	std::unordered_map<std::string,ID> name2id;

public:
	TrackGraph()
		: backing_graph()
		, name2id()
	{}

	TrackGraph& operator=(const TrackGraph&) = default;
	TrackGraph(const TrackGraph&) = default;
	TrackGraph(TrackGraph&&) = default;

	ID getOrCreateNode(const std::string& name) {
		const auto& find_result = name2id.find(name);
		if (find_result == name2id.end()) {
			ID id(boost::add_vertex(backing_graph));
			name2id.emplace(name,id);
			return id;
		} else {
			return (*find_result).second;
		}
	}

	void addConnection(ID id1, ID id2, uint weight) {
		boost::add_edge(id1.get_value(), id2.get_value(), weight, backing_graph);
	}
};

} // end namespace Graph

#endif /* GRAPH_H */
