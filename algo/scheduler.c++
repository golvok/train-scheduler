#include "scheduler.h++"

#include <graphics/graphics.h++>
#include <util/graph_utils.h++>
#include <util/iteration_utils.h++>
#include <util/logging.h++>
#include <util/routing_utils.h++>

#include <boost/property_map/function_property_map.hpp>
#include <iostream>
#include <unordered_set>

namespace algo {

class Scheduler {
	using EdgeWantedCapacities = std::vector<float>;
	using TrainsGoToVertex = std::vector<bool>;
	using Route = std::vector<TrackNetwork::ID>;

	const TrackNetwork& network;
	const TrackNetwork::BackingGraphType& g;
	const std::vector<Passenger>& passengers;

public:
	Scheduler(
		const TrackNetwork& network,
		const std::vector<Passenger>& passengers
	)
	: network(network)
	, g(network.g())
	, passengers(passengers)
	{ }

	/**
	 * Main entry-point flow function. Calls all the other ones.
	 */
	Schedule do_schedule();

	/**
	 * Compute the edge wanted capacites & retun. A measure of how
	 * many passengers want to use each edge.
	 */
	EdgeWantedCapacities compute_edge_wanted_capacities();

	/**
	 * Take the edge wanted capacites, and turn that into a set of
	 * train routes.
	 */
	std::vector<Route> make_rotues(
		const EdgeWantedCapacities& edge_wanted_capacities
	);

	/**
	 * Makes one train route, taking into account previously routed trains,
	 * and the edge wanted capacites. The Route element of the tuple will
	 * be empty if there was no helpful route to be found.
	 */
	std::tuple<Route, TrainsGoToVertex, uint> make_train_route(
		const EdgeWantedCapacities& edge_wanted_capacities,
		TrainsGoToVertex&& trains_go_to_vertex,
		uint vertex_covered_count
	);

	/**
	 * Given a current vertex, return a next vertex to travel to, based
	 * on the wanted capacites and other trains.
	 */
	TrackNetwork::ID compute_next_vertex(
		const EdgeWantedCapacities& edge_wanted_capacities,
		const TrainsGoToVertex& trains_go_to_vertex,
		TrackNetwork::ID curr
	);

	void dump_edge_wanted_capacites_to_dout(
		const EdgeWantedCapacities& edge_wanted_capacities,
		DebugLevel::Level level
	);

	void dump_trains_to_dout(
		const std::vector<Route>& trains,
		DebugLevel::Level level
	);
};

/**
 * Entry Point.
 *
 * Calls flow function
 */
Schedule schedule(
	const TrackNetwork& network,
	const std::vector<Passenger>& passengers
) {
	return Scheduler (
		network,
		passengers
	).do_schedule();
}

Schedule Scheduler::do_schedule() {
	auto fnd_routes_indent = dout(DL::INFO).indentWithTitle("finding routes (scheduler2)");

	auto edge_wanted_capacities_sp = util::make_shared(::util::makeEdgeMap<float>(g,float()));
	auto& edge_wanted_capacities = *edge_wanted_capacities_sp;

	edge_wanted_capacities = compute_edge_wanted_capacities();

	graphics::get().trainsArea().displayTNAndWantedCapacities(edge_wanted_capacities_sp);
	graphics::get().waitForPress();

	auto train_routes = make_rotues(edge_wanted_capacities);

	std::vector<Train> trains;

	for(auto route_iai_iter : index_assoc_iterate(train_routes)) {
		trains.emplace_back(route_iai_iter.i(),std::move(*route_iai_iter),0);
	}

	return Schedule("abc",std::move(trains));
}

Scheduler::EdgeWantedCapacities Scheduler::compute_edge_wanted_capacities() {
	auto edge_wanted_capacities = ::util::makeEdgeMap<float>(g,1);

	auto ewc_indent = dout(DL::WC_D1).indentWithTitle("Computing Edge Wanted Capacities");
	for (const Passenger& p : passengers) {
		auto p_indent = dout(DL::WC_D2).indentWithTitle([&](auto&& out){ out << "Passenger " << p.getName(); });
		auto next_iteration_weights = network.makeEdgeWeightMapCopy();
		for (uint iteration_num = 1; true; ++iteration_num) {
			auto p_indent = dout(DL::WC_D2).indentWithTitle([&](auto&& out){ out << "Iteration " << iteration_num; });
			auto iteration_weights = next_iteration_weights; // get ourselves a copy

			auto iteration_weights_f = [&](TrackNetwork::BackingGraphType::edge_descriptor edge_desc) {
				return iteration_weights[network.getEdgeIndex(edge_desc)];
			};
			auto weight_map = boost::make_function_property_map<TrackNetwork::EdgeID, double>(iteration_weights_f);
			auto route_for_p = ::util::get_shortest_route(
				p,
				network,
				weight_map
			);
			::util::print_route(route_for_p,network,dout(DL::WC_D2));
			{
				auto prev_vertex = route_for_p[0];
				bool first = true;
				for (TrackNetwork::ID id : route_for_p) {
					if (first) { first = false; continue; }
					auto edge_desc = boost::edge(prev_vertex,id,g).first;
					auto edge_index = network.getEdgeIndex(edge_desc);

					next_iteration_weights[edge_index] *= 1.2;
					edge_wanted_capacities[edge_index] *= 1 + (0.1/iteration_num);

					prev_vertex = std::move(id);
				}
			}

			if (iteration_num >= 10) {
				break;
			}
		}
	}

	dump_edge_wanted_capacites_to_dout(edge_wanted_capacities,DL::WC_D1);

	return edge_wanted_capacities;
}

void Scheduler::dump_edge_wanted_capacites_to_dout(
	const EdgeWantedCapacities& edge_wanted_capacities,
	DebugLevel::Level level
) {
	auto edge_wc_indent = dout(level).indentWithTitle("Computed edge wanted capacities");
	for (auto edge_desc : make_iterable(edges(g))) {
		auto edge_index = network.getEdgeIndex(edge_desc);
		dout(level) << edge_index << " : " << edge_wanted_capacities[edge_index] << '\n';
	}
}


std::vector<Scheduler::Route> Scheduler::make_rotues(
	const EdgeWantedCapacities& edge_wanted_capacities
) {
	auto routing_indent = dout(DL::INFO).indentWithTitle("making train routes");


	auto trains_go_to_vertex = ::util::makeVertexMap<bool>(g,false);
	uint vertex_covered_count = 0;

	std::vector<Route> routes;

	auto routing_indent2 = dout(DL::TR_D1).indentWithTitle("Traversing");

	while (true) {

		Route new_route;

		std::tie(new_route,trains_go_to_vertex,vertex_covered_count) = make_train_route(
			edge_wanted_capacities,
			std::move(trains_go_to_vertex),
			vertex_covered_count
		);

		if (new_route.empty()) {
			break;
		}

		routes.push_back(new_route);

		if (vertex_covered_count >= num_vertices(g)) {
			break;
		}
	}

	routing_indent2.endIndent();

	dump_trains_to_dout(routes,DL::INFO);

	return routes;
}

std::tuple<Scheduler::Route, Scheduler::TrainsGoToVertex, uint> Scheduler::make_train_route(
	const EdgeWantedCapacities& edge_wanted_capacities,
	TrainsGoToVertex&& trains_go_to_vertex,
	uint vertex_covered_count
) {
	auto train_indent = dout(DL::TR_D2).indentWithTitle("Train");


	Route route;
	route.push_back(network.getTrainSpawnLocation());
	trains_go_to_vertex[network.getTrainSpawnLocation()] = true;
	vertex_covered_count += 1;

	while (true) {
		TrackNetwork::ID next = compute_next_vertex(
			edge_wanted_capacities,
			trains_go_to_vertex,
			route.back()
		);

		if (next == TrackNetwork::ID(-1)) {
			dout(DL::TR_D3) << "didn't find anything. Ending route.\n";
			break;
		} else {
			if (trains_go_to_vertex[next] == false) {
				trains_go_to_vertex[next] = true;
				vertex_covered_count += 1;
			}
			route.push_back(next);
		}
	}

	return std::make_tuple(
		route,
		trains_go_to_vertex,
		vertex_covered_count
	);
}

TrackNetwork::ID Scheduler::compute_next_vertex(
	const EdgeWantedCapacities& edge_wanted_capacities,
	const TrainsGoToVertex& trains_go_to_vertex,
	TrackNetwork::ID curr
) {
	auto vertex_index = dout(DL::TR_D2).indentWithTitle([&](auto&&out){ out << "Vertex " << network.getVertexName(curr); });

	// check if all adjacent vertices already have trains -- cache?
	bool heed_trains_going_to = false;
	for (TrackNetwork::ID v : make_iterable(adjacent_vertices(curr,g))) {
		if (trains_go_to_vertex[v] == false) {
			heed_trains_going_to = true;
			break;
		}
	}

	if (!heed_trains_going_to) {
		dout(DL::TR_D2) << "-- ignoring existing trains --\n";
	}

	TrackNetwork::ID next(-1);
	float best_wanted_capacity = 0;

	for (auto e_desc : make_iterable(out_edges(curr,g))) {
		TrackNetwork::ID target_v = target(e_desc,g);
		dout(DL::TR_D2) << "looking at " << network.getVertexName(target_v) << ", ";

		if (heed_trains_going_to && trains_go_to_vertex[target_v]) {
			dout(DL::TR_D2) << "skipping!\n";
			continue;
		}

		float wc = edge_wanted_capacities[network.getEdgeIndex(e_desc)];
		dout(DL::TR_D2) << "wc = " << wc;
		if (wc > best_wanted_capacity) {
			dout(DL::TR_D2) << " - is best!\n";
			next = target_v;
			best_wanted_capacity = wc;
		}
	}

	return next;
}

void Scheduler::dump_trains_to_dout(
	const std::vector<Route>& trains,
	DebugLevel::Level level
) {
	auto output_indent = dout(level).indentWithTitle("Trains");
	uint i = 1;
	for (auto& route : trains) {
		auto route_indent = dout(level).indentWithTitle([&](auto&&out){ out << "train " << i; });
		::util::print_route(route,network,dout(level));
		++i;
	}
}

} // end namespace algo
