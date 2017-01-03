
#ifndef ALGO__SCHEDULE_TO_GRAPH_ADAPTER_HPP
#define ALGO__SCHEDULE_TO_GRAPH_ADAPTER_HPP

#include <algo/scheduler.h++>
#include <util/graph_utils.h++>
#include <util/location_id.h++>
#include <util/print_printable.h++>
#include <util/track_network.h++>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/astar_search.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

namespace algo {

class ScheduleToGraphAdapter;

namespace detail {
namespace STGA {

class vertex_descriptor : public ::util::print_with_printable<const TrackNetwork> {
	friend class ::algo::ScheduleToGraphAdapter;

	TrackNetwork::NodeID vertex;
	TrackNetwork::Time time;
	LocationID location;

	vertex_descriptor(TrackNetwork::NodeID v, TrackNetwork::Time t, LocationID location)
		: vertex(v), time(t), location(location)
	{ }
public:
	vertex_descriptor(TrackNetwork::NodeID v, TrackNetwork::Time t, const TrackNetwork& tn)
		: vertex(v), time(t), location(tn.getStationIDByVertexID(v))
	{ }
	vertex_descriptor(TrackNetwork::NodeID v, TrackNetwork::Time t, RouteID route_id, TrainIndex train_index)
		: vertex(v), time(t), location(::util::make_id<TrainID>(route_id,train_index))
	{ }
	vertex_descriptor() : vertex(-1), time(-1), location() { }

	vertex_descriptor(const vertex_descriptor&) = default;
	vertex_descriptor(vertex_descriptor&&) = default;

	vertex_descriptor& operator=(const vertex_descriptor&) = default;
	vertex_descriptor& operator=(vertex_descriptor&&) = default;

	decltype(vertex) getVertex() const { return vertex; }
	decltype(time) getTime() const { return time; }
	decltype(location) getLocation() const { return location; }

	bool operator==(const vertex_descriptor& rhs) const {
		return
			     vertex == rhs.vertex
			&&     time == rhs.time
			&& location == rhs.location
		;
	}

	bool operator!=(const vertex_descriptor& rhs) const { return !(*this == rhs); }

	void print(std::ostream& os, const TrackNetwork& tn) const;
};

std::ostream& operator<<(std::ostream& os, const vertex_descriptor& vd);

using degree_size_type = size_t;

class out_edge_iterator :
	public boost::iterator_facade<
		out_edge_iterator,
		std::pair<vertex_descriptor,vertex_descriptor>,
		boost::forward_traversal_tag,
		std::pair<vertex_descriptor,vertex_descriptor>
	> {
private:
	friend class ::algo::ScheduleToGraphAdapter;

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

	void update_sink_vd();
};

} // end namespace STGA
} // end namespace detail
} // end namespace algo

namespace std {
	template <>
	struct hash<::algo::detail::STGA::vertex_descriptor> {
		size_t operator()(const ::algo::detail::STGA::vertex_descriptor& vd) const {
			return
				  std::hash<decltype(vd.getVertex())>()(vd.getVertex())
				^ std::hash<decltype(vd.getTime())>()(vd.getTime())
				^ std::hash<decltype(vd.getLocation().getValue())>()(vd.getLocation().getValue())
			;
		}
	};
}

namespace algo{

class ScheduleToGraphAdapter {
private:
	friend class ::algo::detail::STGA::vertex_descriptor;
	friend class ::algo::detail::STGA::out_edge_iterator;

	const TrackNetwork& tn;
	const Schedule& sch;

	TrackNetwork::Time station_lookahead_quantum;
public:
	ScheduleToGraphAdapter(const TrackNetwork& tn, const Schedule& sch, TrackNetwork::Time station_lookahead_quantum)
		: tn(tn)
		, sch(sch)
		, station_lookahead_quantum(station_lookahead_quantum)
	{ }

	ScheduleToGraphAdapter(const ScheduleToGraphAdapter&) = default;
	ScheduleToGraphAdapter(ScheduleToGraphAdapter&&) = default;

	ScheduleToGraphAdapter& operator=(const ScheduleToGraphAdapter&) = delete;
	ScheduleToGraphAdapter& operator=(ScheduleToGraphAdapter&&) = delete;

	// Graph concept requirements

	using vertex_descriptor = ::algo::detail::STGA::vertex_descriptor;

	using edge_descriptor        = std::pair<vertex_descriptor,vertex_descriptor>;
	using directed_category      = boost::directed_tag;
	using edge_parallel_category = boost::allow_parallel_edge_tag;
	using traversal_category     = boost::incidence_graph_tag;

	// IncidenceGraph concept requirements
	using degree_size_type = ::algo::detail::STGA::degree_size_type;
	using out_edge_iterator = ::algo::detail::STGA::out_edge_iterator;

	// misc types

	using backing_colour_map = std::unordered_map<
		ScheduleToGraphAdapter::vertex_descriptor,
		boost::default_color_type
	>;
	using colour_map = boost::associative_property_map<backing_colour_map>;

	template<typename CostType>
	using backing_rank_map = std::unordered_map<
		ScheduleToGraphAdapter::vertex_descriptor,
		CostType
	>;
	template<typename CostType>
	using rank_map = boost::associative_property_map<backing_rank_map<CostType>>;

	using backing_distance_map = ::util::default_map<
		ScheduleToGraphAdapter::vertex_descriptor,
		TrackNetwork::Weight
	>;
	using distance_map = boost::associative_property_map<backing_distance_map>;

	// member funcions

	TrackNetwork::Weight get_edge_weight(const edge_descriptor& ed) const {
		(void)ed;
		return 1;
	}

	auto get_edge_weight_map() const {
		return boost::make_function_property_map<edge_descriptor>([&](const edge_descriptor& ed){
			return this->get_edge_weight(ed);
		});
	}

	size_t get_vertex_index(const vertex_descriptor& vd) const;

	auto get_vertex_index_map() const {
		return boost::make_function_property_map<vertex_descriptor>([=](const vertex_descriptor& vd){
			return this->get_vertex_index(vd);
		});
	}

	backing_colour_map make_backing_colour_map() const;
	colour_map make_colour_map(backing_colour_map& bcm) const;

	template<typename CostType>
	auto make_backing_rank_map() const {
		return backing_rank_map<CostType>();
	}
	template<typename HURISTIC>
	auto make_backing_rank_map(HURISTIC) const {
		return make_backing_rank_map<typename HURISTIC::cost_type>();
	}
	template<typename CostType>
	auto make_rank_map(backing_rank_map<CostType>& bcm) const {
		return rank_map<CostType>(bcm);
	}

	auto make_pred_map() const {
		return ::util::make_writable_function_proprety_map<vertex_descriptor>(
			[] (const vertex_descriptor& vd) -> vertex_descriptor {
				return vd;
			}
		);
	}

	auto make_backing_distance_map(const vertex_descriptor& start_vd) const {
		using bdm_mapped_type = backing_distance_map::mapped_type;
		auto bdm = backing_distance_map(
			std::numeric_limits<bdm_mapped_type>::has_infinity
				? std::numeric_limits<bdm_mapped_type>::infinity()
				: std::numeric_limits<bdm_mapped_type>::max()
		);
		bdm[start_vd] = 0;
		return bdm;
	}
	auto make_distance_map(backing_distance_map& bdm) const {
		return distance_map(bdm);
	}

	auto& getTrackNetwork() const { return tn; }
	auto& getSchedule() const { return sch; }

private:
	vertex_descriptor getConnectingVertex(
		vertex_descriptor src,
		degree_size_type out_edge_index
	) const;
};

template<typename STREAM>
STREAM& pretty_print(STREAM&& os, const ::algo::detail::STGA::vertex_descriptor& vd, const TrackNetwork& tn) {
	os << '{' << tn.getVertexName(vd.getVertex()) << "@t=" << vd.getTime() << ",l=" << vd.getLocation() << '}';
	return os;
}

// IncidenceGraph concept requirements
std::pair<ScheduleToGraphAdapter::out_edge_iterator,
ScheduleToGraphAdapter::out_edge_iterator> out_edges(ScheduleToGraphAdapter::vertex_descriptor v, ScheduleToGraphAdapter const& g);
ScheduleToGraphAdapter::degree_size_type out_degree(ScheduleToGraphAdapter::vertex_descriptor v, ScheduleToGraphAdapter const& g);
ScheduleToGraphAdapter::vertex_descriptor source(ScheduleToGraphAdapter::edge_descriptor e, ScheduleToGraphAdapter const& g);
ScheduleToGraphAdapter::vertex_descriptor target(ScheduleToGraphAdapter::edge_descriptor e, ScheduleToGraphAdapter const& g);

// Support for distance as an "internal" property
auto get(
	boost::edge_weight_t,
	const ::algo::ScheduleToGraphAdapter& stga
) -> decltype(stga.get_edge_weight_map());

auto get(
	boost::edge_weight_t,
	const ::algo::ScheduleToGraphAdapter& stga,
	const ::algo::ScheduleToGraphAdapter::edge_descriptor& edge
) -> decltype(stga.get_edge_weight(edge));

auto get(
	boost::vertex_index_t,
	const ::algo::ScheduleToGraphAdapter& stga
) -> decltype(stga.get_vertex_index_map());

auto get(
	boost::vertex_index_t,
	const ::algo::ScheduleToGraphAdapter& stga,
	const ::algo::ScheduleToGraphAdapter::vertex_descriptor& vd
) -> decltype(stga.get_vertex_index(vd));

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

	template<>
	struct property_map<::algo::ScheduleToGraphAdapter, edge_weight_t> {
		using type = decltype (
			std::declval<::algo::ScheduleToGraphAdapter>().get_edge_weight_map()
		);
		using const_type = type;
	};

	template<>
	struct property_map<::algo::ScheduleToGraphAdapter, vertex_index_t> {
		using type = decltype (
			std::declval<::algo::ScheduleToGraphAdapter>().get_vertex_index_map()
		);
		using const_type = type;
	};

	template<>
	struct property_map<::algo::ScheduleToGraphAdapter, vertex_color_t> {
		using type = ::algo::ScheduleToGraphAdapter::colour_map;
		using const_type = type;
	};

}

namespace std {
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
