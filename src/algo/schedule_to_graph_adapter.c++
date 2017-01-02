#include "schedule_to_graph_adapter.h++"

#include <util/logging.h++>

namespace algo {

namespace detail {
	namespace STGA {
		std::ostream& operator<<(std::ostream& os, const vertex_descriptor& vd) {
			os << '{' << vd.getVertex() << "@t=" << vd.getTime() << ", l=" << vd.getLocation() << '}';
			return os;
		}

		std::ostream& operator<<(std::ostream& os, const std::tuple<const vertex_descriptor&, const TrackNetwork&>& pair) {
			const auto& tn = std::get<1>(pair);
			const auto& vd = std::get<0>(pair);
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

	dout(DL::PR_D4) << "constructing " << std::tie(src,tn) << "'s " << out_edge_index << " out edge\n";
	if (out_edge_index == STGA::out_edge_iterator::END_VAL) {
		dout(DL::PR_D4) << "\twas end edge\n";
		return STGA::vertex_descriptor();
	}

	auto print_edge_first = [&](auto&& next_vd) {
		dout(DL::PR_D4) << "\tedge is " << std::tie(src,tn) << " --(#" << out_edge_index << ")-> " << std::tie(next_vd,tn) << '\n';
		return next_vd;
	};

	if (src.getLocation().isTrain()) {
		// if this vertex is a train, return the current station,
		// the next place the train is going. it must be second because
		// it might not have a vertex to return (if this is the train's end)
		if (out_edge_index == STGA::out_edge_iterator::BEGIN_VAL) {
			// return a station vertex
			STGA::vertex_descriptor next_vd (
				src.getVertex(),
				src.getTime() + 1, // TODO get delay as a function of station & train
				tn.getStationIDByVertexID(src.getVertex())
			);
			return print_edge_first(next_vd);
		} else if (out_edge_index == STGA::out_edge_iterator::BEGIN_VAL + 1) {
			// return the next place the train is going (or not if it isn't)
			const auto& train_route = sch.getTrainRoute(src.getLocation().asTrainID().getRouteID());
			const auto& train = train_route.makeTrainFromIndex(src.getLocation().asTrainID().getTrainIndex());
			const auto& next_it = train.getNextPathIterAfterTime(src.getTime(), tn);

			if (next_it != train_route.getPath().end()) {
				using std::next;
				STGA::vertex_descriptor next_vd(
					*next_it,
					train.getExpectedArrivalTime(
						next_it,
						tn
					),
					src.getLocation().asTrainID()
				);
				return print_edge_first(next_vd);
			}
		}
	} else if (src.getLocation().isStation()) {

		STGA::degree_size_type current_out_edge_index = STGA::out_edge_iterator::BEGIN_VAL;

		for (const auto& train_route : sch.getTrainRoutes()) {
			dout(DL::PR_D4) << "checking route " << train_route.getID() << "\n";

			// now find each train that does,
			// and return the out_edge_index'th one

			for (size_t number_of_matches_to_skip = 0;; ++number_of_matches_to_skip) {
				// support for routes that go through stations more than once
				dout(DL::PR_D4) << "skipping " << number_of_matches_to_skip << " matches\n";

				auto trains_in_interval = train_route.getTrainsAtVertexInInterval(
					src.getVertex(),
					std::make_pair(src.getTime(), src.getTime() + station_lookahead_quantum),
					number_of_matches_to_skip,
					tn
				);

				// if nothing was returned, then there will be no more arrivals
				// of this train in the time interval
				if (begin(trains_in_interval) == end(trains_in_interval)) {
					break;
				}

				for (const auto& train_and_arrival : trains_in_interval) {
					if (train_and_arrival.arrival < 0) {
						::util::print_and_throw<std::runtime_error>([&](auto&& err) {
							err << "time went backwards!: " << std::tie(src,tn) << " -> t=" << train_and_arrival.arrival;
						});
					}

					if (out_edge_index == current_out_edge_index) {
						STGA::vertex_descriptor train_vd (
							src.getVertex(),
							train_and_arrival.arrival,
							train_and_arrival.train.getTrainID()
						);
						return print_edge_first(train_vd);
					} else {
						current_out_edge_index += 1;
					}
				}
			}
		}

		if (current_out_edge_index == out_edge_index) {
			// if we get here, there are no more trains in this quantum
			// so return a station vertex, with t+=quantum

			STGA::vertex_descriptor here_again_vd (
				src.getVertex(),
				src.getTime() + station_lookahead_quantum,
				src.getLocation()
			);
			return print_edge_first(here_again_vd);
		}
	} else {
		::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
			str << "unexpected location type\n";
		});
	}

	dout(DL::PR_D4) << "\twas end edge\n";
	return STGA::vertex_descriptor(); // return end node if nothing found.
}

size_t STGA::get_vertex_index(const vertex_descriptor& vd) const {
	// const auto vii = calcVertexIndexInfo(vd, tn);
	// const auto index = vii.index_in_timeslot + vii.num_per_timeslot*vd.getTime();
	// std::cout << "mapping " << std::tie(vd,tn) << " to " << index << '\n';
	(void)vd;
	// TODO?: modify edge generation to produce edges to stations reachable by trains
	// only - and store route/train info *on the edges instead*
	throw "unsupported - no way to provide index for vertexes that are on trains";
	return -1;
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
