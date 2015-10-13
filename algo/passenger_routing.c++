#include "passenger_routing.h++"

#include <algo/schedule_to_graph_adapter.h++>
#include <algo/scheduler.h++>
#include <util/graph_utils.h++>
#include <util/logging.h++>

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
			dout(DL::PR_D3) << "Exploring " << vd << "..." << '\n';
			if(vd.getVertex() == goal_vertex) {
				throw found_goal{vd};
			}
		}
	};
} // end anonymous namespace

PassengerRoutes route_passengers(
	const TrackNetwork& tn,
	const Schedule& sch,
	const std::vector<Passenger>& passgrs
) {
	// { boost::concepts::IncidenceGraphConcept<ScheduleToGraphAdapter> c; (void)c; }

	auto rp_indent = dout(DL::PR_D1).indentWithTitle("Passenger Routing");

	PassengerRoutes results;

	const ScheduleToGraphAdapter baseGraph(tn,sch);

	for (auto passenger : passgrs) {
		auto pass_indent = dout(DL::PR_D1).indentWithTitle([&](auto&& s){ s << "Passenger " << passenger.getName(); });

		// TODO: change next line to use actual leave time when TR and WC understand time...
		auto start_vertex_and_time = STGA::vertex_descriptor(passenger.getEntryId(),0);
		auto goal_vertex = passenger.getExitId();

		auto heuristic = ::util::make_astar_heuristic<ScheduleToGraphAdapter>(
			[&](const STGA::vertex_descriptor& vd) -> TrackNetwork::Weight {
				(void)vd;
				return 1;
			}
		);

		pretty_print(dout(DL::PR_D2) << "Start vertex and time: ",start_vertex_and_time,tn) << '\n';
		dout(DL::PR_D2) << "Goal vertex: " << tn.getVertexName(goal_vertex) << '\n';

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

			STGA::vertex_descriptor prev = fg.vd; // start at the end

			std::vector<TrackNetwork::ID> path { prev.getVertex() };

			pretty_print(dout(DL::PR_D1) << "path found: ",prev,tn);

			while (true) {
				STGA::vertex_descriptor vd = get(pred_map,prev);
				if(!path.empty() && prev == vd) {
					break;
				}
				path.push_back(vd.getVertex());
				pretty_print(dout(DL::PR_D1) << " <- ",vd,tn);
				prev = vd;
			}

			std::reverse(path.begin(),path.end()); // was backwards

			dout(DL::PR_D1) << '\n';

			results.addRoute(passenger, std::move(path));

			continue;
		}

		pretty_print(dout(DL::WARN) << "Didn't find a path from ",start_vertex_and_time,tn) << '\n';
	}

	return results;
}

} // end namespace algo
