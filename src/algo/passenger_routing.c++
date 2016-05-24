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

	// exception type in the case of a no-route
	struct no_route {
	};

	// visitor that causes the algo to terminate when we find the goal vertex (ignores time)
	class astar_goal_visitor : public boost::default_astar_visitor {
	private:
		TrackNetwork::Time start_time;
		TrackNetwork::NodeID goal_vertex;
	public:
		astar_goal_visitor(TrackNetwork::Time start, TrackNetwork::NodeID goal)
			: start_time(start)
			, goal_vertex(goal)
		{ }

		void examine_vertex(STGA::vertex_descriptor vd, ScheduleToGraphAdapter const& g) {
			(void)g;
			dout(DL::PR_D3) << "Exploring " << std::tie(vd,g.getTrackNetwork()) << "..." << '\n';
			if (vd.getVertex() == goal_vertex && vd.getLocation().isStation()) {
				throw found_goal{vd};
			} else if (vd.getTime() > (start_time + 1000)) {
				throw no_route();
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

	RouteTroughScheduleCacheHandle cache_handle;
	for (auto passenger : passgrs) {
		auto pass_indent = dout(DL::PR_D1).indentWithTitle([&](auto&& s){ s << "Passenger " << passenger.getName(); });

		PassengerRoutes::RouteType route;
		std::tie(route, cache_handle) = route_through_schedule(tn, sch, passenger.getEntryID(), passenger.getExitID(), std::move(cache_handle));
		results.addRoute(passenger, std::move(route));
	}

	return results;
}

struct RouteTroughScheduleCache {
	// uh... empty for now
	// if the STGA is cached (which is pointless?), or paths are, then there
	// would have to be some checksum type thing on the Schedule & TN...
};

// These are declared here because the need to be after the definition of RouteTroughScheduleCache
RouteTroughScheduleCacheHandle::RouteTroughScheduleCacheHandle(RouteTroughScheduleCacheHandle&&) = default;
RouteTroughScheduleCacheHandle& RouteTroughScheduleCacheHandle::operator=(RouteTroughScheduleCacheHandle&&) = default;
RouteTroughScheduleCacheHandle::RouteTroughScheduleCacheHandle() = default;
RouteTroughScheduleCacheHandle::~RouteTroughScheduleCacheHandle() { }

std::pair<
	PassengerRoutes::RouteType,
	RouteTroughScheduleCacheHandle
> route_through_schedule(
	const TrackNetwork& tn,
	const Schedule& sch,
	const TrackNetwork::NodeID start_vertex,
	const TrackNetwork::NodeID goal_vertex,
	RouteTroughScheduleCacheHandle&& cache_handle
) {

	// if null, create a cache
	if (!cache_handle) { cache_handle = std::make_unique<::algo::RouteTroughScheduleCache>(); }

	// TODO: change next line to use actual leave time when TR and WC understand time...
	auto start_vertex_and_time = STGA::vertex_descriptor(start_vertex,0,tn);

	auto heuristic = ::util::make_astar_heuristic<ScheduleToGraphAdapter>(
		[&](const STGA::vertex_descriptor& vd) -> TrackNetwork::Weight {
			(void)vd;
			return 1;
		}
	);

	dout(DL::PR_D2) << "Start vertex and time: " << std::tie(start_vertex_and_time,tn) << '\n';
	dout(DL::PR_D2) << "Goal vertex: " << tn.getVertexName(goal_vertex) << '(' << goal_vertex << ")\n";

	const ScheduleToGraphAdapter baseGraph(tn,sch,5); // TODO calculate station_lookahead_quanta from... something

	auto pred_map = baseGraph.make_pred_map();
	auto backing_distance_map = baseGraph.make_backing_distance_map(start_vertex_and_time);
	auto backing_rank_map = baseGraph.make_backing_rank_map(heuristic);
	auto backing_colour_map = baseGraph.make_backing_colour_map();

	try {
		astar_search_no_init(baseGraph,
			start_vertex_and_time,
			heuristic
			, visitor(astar_goal_visitor(start_vertex_and_time.getTime(), goal_vertex))
			. distance_map(baseGraph.make_distance_map(backing_distance_map))
			. predecessor_map(std::ref(pred_map)) // don't want to pass by value
			. rank_map(baseGraph.make_rank_map(backing_rank_map))
			. color_map(baseGraph.make_colour_map(backing_colour_map))
		);
	} catch (const found_goal& fg) { // found a path to the goal
		return { extract_coalesced_path(start_vertex_and_time, fg.vd, pred_map, tn), std::move(cache_handle) };
	} catch (const no_route&) {
		// do nothing
	}

	dout(DL::PR_D1) << "Didn't find a path from " << std::tie(start_vertex_and_time,tn) << " to " << tn.getVertexName(goal_vertex) << '\n';

	return { PassengerRoutes::RouteType(), std::move(cache_handle) };
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
	dout(DL::PR_D3) << std::tie(prev,tn);

	while (true) {
		STGA::vertex_descriptor vd = get(pred_map,prev);
		if(prev == vd) { // we have reached the beginning
			prev = vd;
			break;
		}

		if (
			   (vd.getLocation().isStation() && prev.getLocation().isStation())
			&& (vd.getLocation() != prev.getLocation())
		) {
			::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << "going from a Station to a different Station: " << std::tie(prev,tn) << " <- " << std::tie(vd,tn);
			});
		}
		if (
			   (vd.getLocation().isTrain() && prev.getLocation().isTrain())
			&& (vd.getLocation() != prev.getLocation())
		) {
			::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << "going from a Train to a different Train: " << std::tie(prev,tn) << " <- " << std::tie(vd,tn);
			});
		}
		if (vd.getTime() > prev.getTime()) {
			::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << "time went backwards!: " << std::tie(prev,tn) << " <- " << std::tie(vd,tn);
			});
		}

		// we want to keep the first one (time-wise), so keep the last one we see
		// (keep replacing until the location changes);
		if (vd.getLocation() == path.back().getLocation()) {
			path.back() = PassengerRoutes::RouteElement(vd.getLocation(), vd.getTime());
		} else {
			path.emplace_back(vd.getLocation(), vd.getTime());
		}
		dout(DL::PR_D3) << " <- " << std::tie(vd,tn);
		prev = vd;
	}
	dout(DL::PR_D3) << "\n\t = ";

	if (prev != start) {
		dout(DL::WARN) << "begin vertex is not the start!\n";
	}

	std::reverse(path.begin(),path.end()); // was backwards

	::util::print_route_of_route_elements(path, tn, dout(DL::PR_D1));
	dout(DL::PR_D1) << '\n';

	return path;
}

} // end anonymous namespace

} // end namespace algo
