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

	struct PredecessorMap {
		PredecessorMap() : m() {}
		PredecessorMap(PredecessorMap const& that) : m(that.m) {}

		using key_type       = ScheduleToGraphAdapter::vertex_descriptor;
		using value_type     = ScheduleToGraphAdapter::vertex_descriptor;
		using reference_type = ScheduleToGraphAdapter::vertex_descriptor&;
		using category       = boost::read_write_property_map_tag;

		std::unordered_map<ScheduleToGraphAdapter::vertex_descriptor,ScheduleToGraphAdapter::vertex_descriptor> m;
	};

	STGA::vertex_descriptor get(PredecessorMap const& pm, STGA::vertex_descriptor vd) {
		auto find_results = pm.m.find(vd);
		return (find_results != pm.m.end()) ? find_results->second : vd;
	}

	void put(PredecessorMap & pm, STGA::vertex_descriptor key, STGA::vertex_descriptor value) {
		pm.m[key] = value;
	}

} // end anonymous namespace

int route_passengers(
	const TrackNetwork& tn,
	const Schedule& sch,
	const std::vector<Passenger>& passgrs
) {
	// { boost::concepts::IncidenceGraphConcept<ScheduleToGraphAdapter> c; (void)c; }

	auto rp_indent = dout(DL::PR_D1).indentWithTitle("Passenger Routing");

	const ScheduleToGraphAdapter baseGraph(tn,sch);

	for (auto passenger : passgrs) {
		auto pass_indent = dout(DL::PR_D1).indentWithTitle([&](auto&& s){ s << "Passenger " << passenger.getName(); });

		// TODO: change next line to use actual leave time when TR and WC understand time...
		auto start_vertex_and_time = STGA::vertex_descriptor(passenger.getEntryId(),0);
		auto goal_vertex = passenger.getExitId();

		auto heuristic = ::util::make_astar_heuristic<ScheduleToGraphAdapter>(
			[&](const STGA::vertex_descriptor& vd) -> unsigned {
				(void)vd;
				return 1;
			}
		);

		pretty_print(dout(DL::PR_D2) << "Start vertex and time: ",start_vertex_and_time,tn) << '\n';
		dout(DL::PR_D2) << "Goal vertex: " << tn.getVertexName(goal_vertex) << '\n';

		PredecessorMap pred_map;
		typedef boost::associative_property_map< ::util::default_map<STGA::vertex_descriptor,unsigned> > DistanceMap;
		typedef ::util::default_map<STGA::vertex_descriptor,unsigned> WrappedDistanceMap;
		WrappedDistanceMap wrappedMap = WrappedDistanceMap(std::numeric_limits<unsigned>::max());
		wrappedMap[start_vertex_and_time] = 0;
		DistanceMap d = DistanceMap(wrappedMap);
		auto backing_rank_map = baseGraph.make_backing_rank_map<decltype(heuristic)::cost_type>();
		auto backing_colour_map = baseGraph.make_backing_colour_map();

		STGA::vertex_descriptor end_vertex_and_time;

		try {
			astar_search_no_init(baseGraph,
				start_vertex_and_time,
				heuristic
				, visitor(astar_goal_visitor(goal_vertex))
				. distance_map(d)
				. predecessor_map(boost::ref(pred_map))
				. rank_map(baseGraph.make_rank_map(backing_rank_map))
				. color_map(baseGraph.make_colour_map(backing_colour_map))
				. distance_compare(std::less<unsigned>())
				. distance_combine(std::plus<unsigned>())
			);
		} catch(found_goal const& fg) { // found a path to the goal
			end_vertex_and_time = fg.vd;

			std::list<STGA::vertex_descriptor> shortest_path;
			for(STGA::vertex_descriptor vd = end_vertex_and_time;; vd = get(pred_map,vd)) {
				shortest_path.push_front(vd);
				if(get(pred_map,vd) == vd)
					break;
			}
			pretty_print(dout(DL::PR_D1) << "Shortest path from ",start_vertex_and_time,tn) << " to "
				<< tn.getVertexName(goal_vertex) << ": ";
			std::list<STGA::vertex_descriptor>::iterator spi = shortest_path.begin();
			pretty_print(dout(DL::PR_D1),start_vertex_and_time,tn);
			for(++spi; spi != shortest_path.end(); ++spi)
				pretty_print(dout(DL::PR_D1) << " -> ",*spi,tn);
			dout(DL::PR_D1) << '\n';
			continue;
		}

		pretty_print(dout(DL::PR_D1) << "Didn't find a path from ",start_vertex_and_time,tn) << '\n';
	}

	return 0;
}

} // end namespace algo
