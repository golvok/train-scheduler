
#ifndef ALGO__SCHEDULE_TO_GRAPH_ADAPTER_HPP
#define ALGO__SCHEDULE_TO_GRAPH_ADAPTER_HPP

#include <util/track_network.h++>
#include <algo/scheduler.h++>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/astar_search.hpp>
#include <iostream>

namespace algo {

class ScheduleToGraphAdapter {
private:
	const TrackNetwork& tn;
	const Schedule& sch;
public:
	ScheduleToGraphAdapter(const TrackNetwork& tn, const Schedule& sch)
		: tn(tn)
		, sch(sch)
	{ }

	ScheduleToGraphAdapter(const ScheduleToGraphAdapter&) = default;
	ScheduleToGraphAdapter(ScheduleToGraphAdapter&&) = default;

	ScheduleToGraphAdapter& operator=(const ScheduleToGraphAdapter&) = delete;
	ScheduleToGraphAdapter& operator=(ScheduleToGraphAdapter&&) = delete;

	// Graph concept requirements

	class vertex_descriptor {
		TrackNetwork::ID vertex;
		TrackNetwork::Time time;
	public:
		vertex_descriptor() : vertex(-1), time(-1) { }
		vertex_descriptor(TrackNetwork::ID v, TrackNetwork::Time t)	: vertex(v), time(t) { }

		vertex_descriptor(const vertex_descriptor&) = default;
		vertex_descriptor(vertex_descriptor&&) = default;

		vertex_descriptor& operator=(const vertex_descriptor&) = default;
		vertex_descriptor& operator=(vertex_descriptor&&) = default;

		decltype(vertex) getVertex() const { return vertex; }
		decltype(time) getTime() const { return time; }

		bool operator==(const vertex_descriptor& rhs) const {
			return
				   vertex == rhs.vertex
				&&   time == rhs.time
			;
		}

		bool operator!=(const vertex_descriptor& rhs) const { return !(*this == rhs); }

	};

	using edge_descriptor        = std::pair<vertex_descriptor,vertex_descriptor>;
	using directed_category      = boost::directed_tag;
	using edge_parallel_category = boost::allow_parallel_edge_tag;
	using traversal_category     = boost::incidence_graph_tag;

	// IncidenceGraph concept requirements

	using degree_size_type = size_t;
	class out_edge_iterator :
		public boost::iterator_facade<
			out_edge_iterator,
			std::pair<vertex_descriptor,vertex_descriptor>,
			boost::forward_traversal_tag,
			std::pair<vertex_descriptor,vertex_descriptor>
		> {
	private:
		ScheduleToGraphAdapter const* stga;
		vertex_descriptor src_vd;
		vertex_descriptor sink_vd;
		degree_size_type out_edge_index;

	public:

		static const degree_size_type BEGIN_VAL = 0;
		static const degree_size_type END_VAL = -1;

		out_edge_iterator() : stga(nullptr), src_vd(), sink_vd(), out_edge_index(-1) { }

		out_edge_iterator(
			vertex_descriptor src_vd,
			ScheduleToGraphAdapter const* stga,
			degree_size_type out_edge_index
		)
			: stga(stga)
			, src_vd(src_vd)
			, sink_vd()
			, out_edge_index(out_edge_index)
		{
			update_sink_vd();
		}

		out_edge_iterator(const out_edge_iterator&) = default;
		out_edge_iterator(out_edge_iterator&&) = default;

		out_edge_iterator& operator=(out_edge_iterator const&) = default;
		out_edge_iterator& operator=(out_edge_iterator&&) = default;

		std::pair<vertex_descriptor,vertex_descriptor> operator*() const {
			return std::make_pair(src_vd, sink_vd);
		}

		out_edge_iterator& operator++() {
			out_edge_index += 1;
			update_sink_vd();
			return *this;
		}

		bool operator==(const out_edge_iterator& rhs) const {
			return
				           src_vd == rhs.src_vd
				&&        sink_vd == rhs.sink_vd
				&&           stga == rhs.stga
				&& out_edge_index == rhs.out_edge_index
			;
		}

		bool operator!=(const out_edge_iterator& rhs) const { return !(*this == rhs); }

		bool equal(out_edge_iterator const& rhs) const { return *this == rhs; }
		void increment() { operator++(); }

		void update_sink_vd() {
			sink_vd = stga->getConnectingVertex(src_vd,out_edge_index);
			if (sink_vd == ScheduleToGraphAdapter::vertex_descriptor()) {
				out_edge_index = END_VAL;
			}
		}

	};

private:
	vertex_descriptor getConnectingVertex(
		vertex_descriptor src,
		degree_size_type out_edge_index
	) const;
};

std::ostream& operator<<(std::ostream& os, const ScheduleToGraphAdapter::vertex_descriptor& vd);

template<typename STREAM>
STREAM& pretty_print(STREAM&& os, const ScheduleToGraphAdapter::vertex_descriptor& vd, const TrackNetwork& tn) {
	os << '{' << tn.getVertexName(vd.getVertex()) << "@t=" << vd.getTime() << '}';
	return os;
}

// IncidenceGraph concept requirements
std::pair<ScheduleToGraphAdapter::out_edge_iterator,
ScheduleToGraphAdapter::out_edge_iterator> out_edges(ScheduleToGraphAdapter::vertex_descriptor v, ScheduleToGraphAdapter const& g);
ScheduleToGraphAdapter::degree_size_type out_degree(ScheduleToGraphAdapter::vertex_descriptor v, ScheduleToGraphAdapter const& g);
ScheduleToGraphAdapter::vertex_descriptor source(ScheduleToGraphAdapter::edge_descriptor e, ScheduleToGraphAdapter const& g);
ScheduleToGraphAdapter::vertex_descriptor target(ScheduleToGraphAdapter::edge_descriptor e, ScheduleToGraphAdapter const& g);

} // namespace algo

namespace boost {
	template <> struct graph_traits<::algo::ScheduleToGraphAdapter> {
		using G = ::algo::ScheduleToGraphAdapter;

		using vertex_descriptor = G::vertex_descriptor;
		using edge_descriptor   = G::edge_descriptor;
		using out_edge_iterator = G::out_edge_iterator;

		using directed_category      = G::directed_category;
		using edge_parallel_category = G::edge_parallel_category;
		using traversal_category     = G::traversal_category;

		using degree_size_type = G::degree_size_type;

		using in_edge_iterator   = void;
		using vertex_iterator    = void;
		using vertices_size_type = void;
		using edge_iterator      = void;
		using edges_size_type    = void;
	};
}

namespace std {
	template <>
	struct hash<::algo::ScheduleToGraphAdapter::vertex_descriptor> {
		size_t operator()(const ::algo::ScheduleToGraphAdapter::vertex_descriptor& vd) const {
			return
				  std::hash<decltype(vd.getVertex())>()(vd.getVertex())
				^ std::hash<decltype(vd.getTime())>()(vd.getTime())
			;
		}
	};

	template <>
	struct hash<::algo::ScheduleToGraphAdapter::edge_descriptor> {
		size_t operator()(const ::algo::ScheduleToGraphAdapter::edge_descriptor& ed) const {
			return
				  std::hash<decltype(ed.first )>()(ed.first )
				^ std::hash<decltype(ed.second)>()(ed.second)
			;
		}
	};
}

#endif /* ALGO__SCHEDULE_TO_GRAPH_ADAPTER_HPP */