#include "schedule_to_graph_adapter.h++"

#include <util/logging.h++>

namespace algo {

using STGA = ScheduleToGraphAdapter;

STGA::vertex_descriptor STGA::getConnectingVertex(
	STGA::vertex_descriptor src,
	STGA::degree_size_type out_edge_index
) const {
	STGA::degree_size_type current_out_edge_index = STGA::out_edge_iterator::BEGIN_VAL;

	for (auto& train : sch.getTrains()) {
		for (
			auto vertex_it = train.getRoute().begin();
			vertex_it != train.getRoute().end();
			++vertex_it
		) {
			// if train goes through here, and it doesn't end here
			if (*vertex_it == src.getVertex() && (vertex_it + 1) != train.getRoute().end()) {
				// if this is the right one, return it
				TrackNetwork::ID next = *(vertex_it + 1);
				if (out_edge_index == current_out_edge_index) {
					STGA::vertex_descriptor next_vd (
						next,
						src.getTime() + boost::get(
							&TrackNetwork::EdgeProperties::weight,
							tn.g(),
							boost::edge(*vertex_it,next,tn.g()).first
						) / train.getSpeed()
					);
					dout(DL::PR_D3) << src << " --(" << out_edge_index << ")-> " << next_vd << '\n';
					return next_vd;
				}

				// otherwise, increment for the next one
				current_out_edge_index += 1;
			}
		}
	}
	return STGA::vertex_descriptor(); // return end node if nothing found.
}

STGA::backing_colour_map STGA::make_backing_colour_map() const {
	return STGA::backing_colour_map();
}

STGA::colour_map STGA::make_colour_map(STGA::backing_colour_map& bcm) const {
	return STGA::colour_map(bcm);
}

std::ostream& operator<<(std::ostream& os, const ScheduleToGraphAdapter::vertex_descriptor& vd) {
	os << '{' << vd.getVertex() << "@t=" << vd.getTime() << '}';
	return os;
}

std::pair<STGA::out_edge_iterator, STGA::out_edge_iterator>
out_edges(
	STGA::vertex_descriptor v,
	STGA const& g
) {
	return std::make_pair(
		STGA::out_edge_iterator(v, &g, STGA::out_edge_iterator::BEGIN_VAL),
		STGA::out_edge_iterator(v, &g, STGA::out_edge_iterator::END_VAL)
	);
}

STGA::degree_size_type
out_degree(
	STGA::vertex_descriptor v,
	STGA const& g
) {
	(void)g;
	(void)v;
	assert(0 && "out_degree cannot be called - can't define properly");
	return STGA::degree_size_type(-1); // this might work... but it's technically infinite.
}

STGA::vertex_descriptor
source(STGA::edge_descriptor e,
	   STGA const& g) {
	(void)g;
	return e.first;
}

STGA::vertex_descriptor target(
	STGA::edge_descriptor e,
	STGA const& g) {
	(void)g;
	return e.second;
}

auto get(
	boost::edge_weight_t,
	const ::algo::ScheduleToGraphAdapter& stga
) -> decltype(stga.get_edge_weight_map()) {
	return stga.get_edge_weight_map();
}

auto get(
	boost::edge_weight_t,
	const ::algo::ScheduleToGraphAdapter& stga,
	const ::algo::ScheduleToGraphAdapter::edge_descriptor& edge
) -> decltype(stga.get_edge_weight(edge)) {
	return stga.get_edge_weight(edge);
}

} // end namespace algo
