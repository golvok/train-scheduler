#include "schedule_to_graph_adapter.h++"

#include <sstream>

#include <util/logging.h++>

namespace algo {

namespace detail {
	namespace STGA {
		// std::ostream& operator<<(std::ostream& os, const vertex_descriptor& vd) {
		// 	os << '{' << vd.getVertex() << "@t=" << vd.getTime() << ", l=" << vd.getLocation() << '}';
		// 	return os;
		// }

		std::ostream& operator<<(std::ostream& os, std::pair<const vertex_descriptor&, const TrackNetwork&> pair) {
			auto& tn = pair.second;
			auto& vd = pair.first;
			os << '{' << tn.getVertexName(vd.getVertex()) << "@t=" << vd.getTime() << ",l=" << vd.getLocation() << '}';
			return os;
		}

		void out_edge_iterator::update_sink_vd() {
			sink_vd = stga->getConnectingVertex(src_vd,out_edge_index);
			if (sink_vd == vertex_descriptor()) {
				out_edge_index = END_VAL;
			}
		}
	}
}

using STGA = ScheduleToGraphAdapter;

STGA::vertex_descriptor STGA::getConnectingVertex(
	STGA::vertex_descriptor src,
	STGA::degree_size_type out_edge_index
) const {
	// NOTE: this function defaults to returning the a default
	// constructed vertex descriptor, which is used as the
	// past-end value so every path must return an actual vertex
	// so that the iteration will not just end after that path
	// fails to return a vertex

	dout(DL::PR_D3) << "constructing " << std::make_pair(src,tn) << "'s " << out_edge_index << " out edge\n";
	if (out_edge_index == STGA::out_edge_iterator::END_VAL) {
		dout(DL::PR_D3) << "\twas end edge\n";
		return STGA::vertex_descriptor();
	}

	if (src.getLocation().isTrain()) {
		// if this vertex is a train, return the current station,
		// the next place the train is going. it must be second because
		// it might not have a vertex to return (if this is the train's end)
		if (out_edge_index == STGA::out_edge_iterator::BEGIN_VAL) {
			// return a station vertex
			STGA::vertex_descriptor next_vd (
				src.getVertex(),
				src.getTime() + 1, // TODO get delay as a function of station & train
				tn.getStationIdByVertexId(src.getVertex())
			);
			dout(DL::PR_D3) << "constructing " << std::make_pair(src,tn) << " --(#" << out_edge_index << ")-> " << std::make_pair(next_vd,tn) << '\n';
			return next_vd;
		} else if (out_edge_index == STGA::out_edge_iterator::BEGIN_VAL + 1) {
			// return the next place the train is going (or not if it isn't)
			const auto& train = sch.getTrain(src.getLocation().asTrainId());
			auto current_it = std::find(train.getRoute().begin(), train.getRoute().end(), src.getVertex());
			if (current_it == train.getRoute().end()) {
				std::ostringstream err;
				err << "vertex " << std::make_pair(src,tn) << " is invalid. It's train doesn't go to it's vertex!\n";
				dout(DL::ERROR) << err.str();
				throw std::invalid_argument(err.str());
			}
			if (current_it + 1 != train.getRoute().end()) {
				auto next = *(current_it + 1);
				STGA::vertex_descriptor next_vd (
					next,
					src.getTime() + boost::get(
						&TrackNetwork::EdgeProperties::weight,
						tn.g(),
						boost::edge(*current_it,next,tn.g()).first
					) / train.getSpeed(),
					train.getId()
				);
				dout(DL::PR_D3) << "constructing " << std::make_pair(src,tn) << " --(#" << out_edge_index << ")-> " << std::make_pair(next_vd,tn) << '\n';
				return next_vd;
			}
		}
	} else if (src.getLocation().isStation()) {

		STGA::degree_size_type current_out_edge_index = STGA::out_edge_iterator::BEGIN_VAL;

		for (auto& train : sch.getTrains()) {
			TrackNetwork::Time time_to_here = 0;
			for (
				auto vertex_it = train.getRoute().begin();
				vertex_it != train.getRoute().end();
				++vertex_it
			) {
				if (vertex_it == train.getRoute().end()) {
					break; // skip last one
				}

				if (vertex_it == train.getRoute().begin()) {
					time_to_here = train.getDepartureTime();
				} else {
					// add time to src vertex, so we know when the train gets to the next vertex
					TrackNetwork::ID prev = *(vertex_it - 1);
					time_to_here += boost::get(
						&TrackNetwork::EdgeProperties::weight,
						tn.g(),
						boost::edge(prev,*vertex_it,tn.g()).first
					) / train.getSpeed();
				}

				// if train goes through here, and it doesn't end here
				if (*vertex_it == src.getVertex()) {
					if (out_edge_index == current_out_edge_index) {
						// if this is the right one, return a vertex corresponding to it
						STGA::vertex_descriptor next_vd (
							*vertex_it,
							time_to_here,
							train.getId()
						);
						dout(DL::PR_D3) << "constructing " << std::make_pair(src,tn) << " --(#" << out_edge_index << ")-> " << std::make_pair(next_vd,tn) << '\n';
						return next_vd;
					}

					// otherwise, increment for the next one
					current_out_edge_index += 1;
				}
			}
		}
	}

	dout(DL::PR_D3) << "\twas end edge\n";
	return STGA::vertex_descriptor(); // return end node if nothing found.
}

STGA::backing_colour_map STGA::make_backing_colour_map() const {
	return STGA::backing_colour_map();
}

STGA::colour_map STGA::make_colour_map(STGA::backing_colour_map& bcm) const {
	return STGA::colour_map(bcm);
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

auto get(
	boost::vertex_index_t,
	const ::algo::ScheduleToGraphAdapter& stga
) -> decltype(stga.get_vertex_index_map()) {
	return stga.get_vertex_index_map();
}

auto get(
	boost::vertex_index_t,
	const ::algo::ScheduleToGraphAdapter& stga,
	const ::algo::ScheduleToGraphAdapter::vertex_descriptor& vd
) -> decltype(stga.get_vertex_index(vd)) {
	return stga.get_vertex_index(vd);
}

} // end namespace algo
