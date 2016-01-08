#include "passenger_routing.h++"

#include <algo/schedule_to_graph_adapter.h++>
#include <algo/scheduler.h++>
#include <util/graph_utils.h++>
#include <util/logging.h++>
#include <util/routing_utils.h++>

#include <list>

using STGA = ::algo::ScheduleToGraphAdapter;

namespace algo {

namespace {
	struct found_goal {
		STGA::vertex_descriptor vd;
	}; // exception for termination

	// visitor that causes the algo to terminate when we find the goal vertex (ignores time)
	class astar_goal_visitor : public boost::default_astar_visitor {
	private:
		TrackNetwork::ID goal_vertex;
	public:
		astar_goal_visitor(TrackNetwork::ID goal) : goal_vertex(goal) {}

		void examine_vertex(STGA::vertex_descriptor vd, ScheduleToGraphAdapter const& g) {
			(void)g;
			dout(DL::PR_D3) << "Exploring " << std::make_pair(vd,g.getTrackNetwork()) << "..." << '\n';
			if(vd.getVertex() == goal_vertex && vd.getLocation().isStation()) {
				throw found_goal{vd};
			}
		}
	};

	template<typename PRED_MAP>
	PassengerRoutes::InternalRouteType extract_coalesced_path(
		const STGA::vertex_descriptor& start,
		const STGA::vertex_descriptor& end,
		const PRED_MAP& pred_map,
		const TrackNetwork& tn
	);
} // end anonymous namespace

PassengerRoutes route_passengers(
	const TrackNetwork& tn,
	const Schedule& sch,
	const PassengerList& passgrs
) {
	// { boost::concepts::IncidenceGraphConcept<ScheduleToGraphAdapter> c; (void)c; }

	auto rp_indent = dout(DL::PR_D1).indentWithTitle("Passenger Routing");

	PassengerRoutes results;

	const ScheduleToGraphAdapter baseGraph(tn,sch,5); // TODO calculate station_lookahead_quanta from... something

	for (auto passenger : passgrs) {
		auto pass_indent = dout(DL::PR_D1).indentWithTitle([&](auto&& s){ s << "Passenger " << passenger.getName(); });

		// TODO: change next line to use actual leave time when TR and WC understand time...
		auto start_vertex_and_time = STGA::vertex_descriptor(passenger.getEntryID(),0,tn);
		auto goal_vertex = passenger.getExitID();

		auto heuristic = ::util::make_astar_heuristic<ScheduleToGraphAdapter>(
			[&](const STGA::vertex_descriptor& vd) -> TrackNetwork::Weight {
				(void)vd;
				return 1;
			}
		);

		dout(DL::PR_D2) << "Start vertex and time: " << std::make_pair(start_vertex_and_time,tn) << '\n';
		dout(DL::PR_D2) << "Goal vertex: " << tn.getVertexName(goal_vertex) << '(' << goal_vertex << ")\n";

		auto pred_map = baseGraph.make_pred_map();
		auto backing_distance_map = baseGraph.make_backing_distance_map(start_vertex_and_time);
		auto backing_rank_map = baseGraph.make_backing_rank_map(heuristic);
		auto backing_colour_map = baseGraph.make_backing_colour_map();

		try {
			astar_search_no_init(baseGraph,
				start_vertex_and_time,
				heuristic
				, visitor(astar_goal_visitor(goal_vertex))
				. distance_map(baseGraph.make_distance_map(backing_distance_map))
				. predecessor_map(std::ref(pred_map)) // don't want to pass by value
				. rank_map(baseGraph.make_rank_map(backing_rank_map))
				. color_map(baseGraph.make_colour_map(backing_colour_map))
			);
		} catch(found_goal const& fg) { // found a path to the goal

			results.addRoute(passenger, extract_coalesced_path(start_vertex_and_time, fg.vd, pred_map, tn));

			continue;
		}

		dout(DL::WARN) << "Didn't find a path from " << std::make_pair(start_vertex_and_time,tn) << '\n';
	}

	return results;
}

namespace {

template<typename PRED_MAP>
PassengerRoutes::InternalRouteType extract_coalesced_path(
	const STGA::vertex_descriptor& start,
	const STGA::vertex_descriptor& end,
	const PRED_MAP& pred_map,
	const TrackNetwork& tn
) {
	STGA::vertex_descriptor prev = end;

	PassengerRoutes::InternalRouteType path;
	path.emplace_back(prev.getLocation(), prev.getTime());

	dout(DL::PR_D1) << "path found: ";
	dout(DL::PR_D3) << std::make_pair(prev,tn);

	while (true) {
		STGA::vertex_descriptor vd = get(pred_map,prev);
		if(prev == vd) { // we have reached the beginning
			prev = vd;
			break;
		}

		if (vd.getLocation().isStation() && prev.getLocation().isStation()) {
			::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << "going from a Station to a Station: " << std::make_pair(prev,tn) << " <- " << std::make_pair(vd,tn);
			});
		}
		if (
			   (vd.getLocation().isTrain() && prev.getLocation().isTrain())
			&& (vd.getLocation().asTrainID() != prev.getLocation().asTrainID())
		) {
			::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << "going from a Train to a different Train: " << std::make_pair(prev,tn) << " <- " << std::make_pair(vd,tn);
			});
		}
		if (vd.getTime() > prev.getTime()) {
			::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << "time went backwards!: " << std::make_pair(prev,tn) << " <- " << std::make_pair(vd,tn);
			});
		}

		// we want to keep the first one (time-wise), so keep the last one we see
		// (keep replacing until the location changes);
		if (vd.getLocation() == path.back().getLocation()) {
			path.back() = PassengerRoutes::RouteElement(vd.getLocation(), vd.getTime());
		} else {
			path.emplace_back(vd.getLocation(), vd.getTime());
		}
		dout(DL::PR_D3) << " <- " << std::make_pair(vd,tn);
		prev = vd;
	}
	dout(DL::PR_D3) << "\n\t = ";

	if (prev != start) {
		dout(DL::WARN) << "begin vertex is not the start!\n";
	}

	std::reverse(path.begin(),path.end()); // was backwards

	::util::print_route(path, dout(DL::PR_D1), [&](auto&& str, auto&& elem) {
		if (elem.getLocation().isStation()) {
			str << '{' << tn.getVertexName(tn.getVertexIDByStationID(elem.getLocation().asStationID()));
		} else {
			str << "{l=" << elem.getLocation();
		}
		str << "@t=" << elem.getTime() << '}';
	});

	return path;
}

} // end anonymous namespace

} // end namespace algo
