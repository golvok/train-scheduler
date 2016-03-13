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

class Scheduler2 {
	using EdgeWantedCapacities = std::vector<float>;
	using TrainsGoToVertex = std::vector<bool>;
	using Route = std::vector<TrackNetwork::ID>;

	const TrackNetwork& network;
	const TrackNetwork::BackingGraphType& g;
	const PassengerList& passengers;

public:
	Scheduler2(
		const TrackNetwork& network,
		const PassengerList& passengers
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

class Scheduler3 {
	using VertexList = std::vector<TrackNetwork::ID>;
	using VertexListList = std::vector<VertexList>;

	const TrackNetwork& network;
	const PassengerList& passengers;
	const uint max_trains_at_a_time;

public:
	Scheduler3(
		const TrackNetwork& network,
		const PassengerList& passengers,
		uint max_trains_at_a_time
	)
	: network(network)
	, passengers(passengers)
	, max_trains_at_a_time(max_trains_at_a_time)
	{ }

	/**
	 * Main entry-point flow function. Calls all the other ones.
	 */
	Schedule do_schedule();

	VertexListList make_one_train_per_passenger();
	VertexListList coalesce_trains(VertexListList&& initial_trains);

	void dump_trains_to_dout(
		const VertexListList& train_routes,
		const std::string& title,
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
	const PassengerList& passengers
) {
	return Scheduler3 (
		network,
		passengers,
		2
	).do_schedule();
}

Schedule Scheduler2::do_schedule() {
	auto fnd_routes_indent = dout(DL::INFO).indentWithTitle("finding routes (scheduler2)");

	auto edge_wanted_capacities_sp = util::make_shared(::util::makeEdgeMap<float>(g,float()));
	auto& edge_wanted_capacities = *edge_wanted_capacities_sp;

	edge_wanted_capacities = compute_edge_wanted_capacities();

	graphics::get().trainsArea().displayTNAndWantedCapacities(edge_wanted_capacities_sp);
	graphics::get().waitForPress();

	auto train_routes = make_rotues(edge_wanted_capacities);

	std::vector<TrainRoute> trains;

	for(auto& train_route : train_routes) {
		auto repeat_time = train_route.size();
		trains.emplace_back(
			::util::make_id<::algo::RouteID>(trains.size()),
			std::move(train_route),
			std::vector<TrackNetwork::Time>{ 0 }, // assumes all routes start at time 0
			repeat_time, // make it repeat after it's done for now
			network
		);
	}

	return Schedule("Scheduler2 schedule",std::move(trains));
}

Scheduler2::EdgeWantedCapacities Scheduler2::compute_edge_wanted_capacities() {
	auto edge_wanted_capacities = ::util::makeEdgeMap<float>(g,1);

	auto ewc_indent = dout(DL::WC_D1).indentWithTitle("Computing Edge Wanted Capacities");
	for (const Passenger& p : passengers) {
		auto p_indent = dout(DL::WC_D2).indentWithTitle([&](auto&& out){ out << "Passenger " << p.getName(); });
		if (p.getStartTime() != 0) {
			::util::print_and_throw<std::invalid_argument>([&](auto&& stream) {
				stream << "don't support passengers with entry time != 0 (" << std::tie(p,network) << ')';
			});
		}
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

void Scheduler2::dump_edge_wanted_capacites_to_dout(
	const EdgeWantedCapacities& edge_wanted_capacities,
	DebugLevel::Level level
) {
	auto edge_wc_indent = dout(level).indentWithTitle("Computed edge wanted capacities");
	for (auto edge_desc : make_iterable(edges(g))) {
		auto edge_index = network.getEdgeIndex(edge_desc);
		dout(level) << edge_index << " : " << edge_wanted_capacities[edge_index] << '\n';
	}
}


std::vector<Scheduler2::Route> Scheduler2::make_rotues(
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

std::tuple<Scheduler2::Route, Scheduler2::TrainsGoToVertex, uint> Scheduler2::make_train_route(
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

TrackNetwork::ID Scheduler2::compute_next_vertex(
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

void Scheduler2::dump_trains_to_dout(
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

/*** ALGO IDEAS ****\
Figure out how to model people going to similar places (like pins on a net)
	maybe some sort of path grouping?
		then the paths need to be split...?

Maybe start with one train/passenger, then iterative coalesce?
	capacity resolution must be done after.
*/

Schedule Scheduler3::do_schedule() {
	auto fnd_routes_indent = dout(DL::INFO).indentWithTitle("finding routes (scheduler3)");

	auto trains = make_one_train_per_passenger();

	dump_trains_to_dout(trains, "Initial Trains", DL::TR_D1);

	trains = coalesce_trains(std::move(trains));

	std::vector<TrainRoute> train_routes;

	for(auto& train : trains) {
		auto repeat_time = train.size();
		train_routes.emplace_back(
			::util::make_id<::algo::RouteID>(train_routes.size()),
			std::move(train),
			std::vector<TrackNetwork::Time>{ 0 }, // assumes all routes start at time 0
			repeat_time, // make it repeat after it's done for now
			network
		);
	}

	return Schedule("Scheduler3 schedule",std::move(train_routes));
}

Scheduler3::VertexListList Scheduler3::make_one_train_per_passenger() {
	VertexListList trains;
	for (const auto& p : passengers) {
		trains.emplace_back(::util::get_shortest_route(
			p.getEntryID(),
			p.getExitID(),
			network
		));
	}
	return std::move(trains);
}

Scheduler3::VertexListList Scheduler3::coalesce_trains(VertexListList&& trains) {

	size_t old_size = trains.size();
	int iter_num = 1;
	while (true) {
		auto iter_indent = dout(DL::TR_D2).indentWithTitle([&](auto&& str) {
			str << "Coalescing Iteration " << iter_num;
		});
		std::vector<bool> train_is_rudundant(trains.size());

		for (size_t train_i = 0; train_i != trains.size(); ++train_i) {
			if (train_is_rudundant[train_i]) { continue; }
			auto& train = trains[train_i];

			for (size_t comp_train_i = train_i + 1; comp_train_i != trains.size(); ++comp_train_i) {
				if (train_is_rudundant[comp_train_i]) { continue; }
				auto& comp_train = trains[comp_train_i];

				// TODO: this DOES NOT handle repeated vertices...

				// the location of the train's first node in comp_train
				auto comp_first_match = std::find(
					comp_train.begin(), comp_train.end(), train.front()
				);

				// the location of the comp_train's first node in train
				auto first_match = std::find(
					train.begin(), train.end(), comp_train.front()
				);

				if (first_match == train.end() && comp_first_match == comp_train.end()) {
					// neither path overlaps with the other
					continue;
				} else {
					// exactly one train first node overlaps,
					// so reset the other to it's beginning, so mismatch will
					// start at where they overlap
					if (first_match == train.end()) {
						first_match = train.begin();
					}

					if (comp_first_match == comp_train.end()) {
						comp_first_match = comp_train.begin();
					}
				}

				auto mismatch_results = std::mismatch(
					first_match, train.end(),
					comp_first_match, comp_train.end()
				);
				bool      reached_end = mismatch_results.first  == train.end();
				bool comp_reached_end = mismatch_results.second == comp_train.end();

				size_t redundant_train = -1;
				if (!reached_end && !comp_reached_end) {
					// paths diverged, so add for consideration for extension (TODO)
					continue;
				} else if (!reached_end && comp_reached_end) {
					if (first_match == train.begin()) {
						// extend comp train? maybe. So, add to consideration. (TODO)
						continue;
					} else {
						// they overlap, and train is longer
						redundant_train = comp_train_i;
					}
				} else if (reached_end && !comp_reached_end) {
					if (comp_first_match == comp_train.begin()) {
						// extend train? maybe. So, add to consideration. (TODO)
						continue;
					} else {
						// they overlap, and comp trian is longer
						redundant_train = train_i;
					}
				} else {
					// both end on the same vertex, so remove the shorter one.
					// (the one that could have it's start vertex found in the other)
					if (first_match == train.begin()) {
						redundant_train = train_i;
					} else if (comp_first_match == comp_train.begin()) {
						redundant_train = comp_train_i;
					}
				}

				if (redundant_train == (size_t)-1) {
					::util::print_and_throw<std::runtime_error>([&](auto&& err) { err << "no train selected as redundant!\n"; });
				}

				train_is_rudundant[redundant_train] = true;

				size_t other_train = redundant_train == train_i ? comp_train_i : train_i;
				dout(DL::TR_D3) << "train #" << redundant_train << " marked redundant in favour of #" << other_train << "\n";
			}
		}

		trains.erase(
			util::remove_by_index(
				trains.begin(), trains.end(),
				[&](auto& index) { return train_is_rudundant[index]; }
			),
			trains.end()
		);

		dump_trains_to_dout(trains, "Trains After Iteration", DL::TR_D2);

		if (trains.size() <= max_trains_at_a_time || trains.size() == old_size) {
			break;
		}
		old_size = trains.size();
		iter_num += 1;
	}

	return std::move(trains);
}

void Scheduler3::dump_trains_to_dout(
	const VertexListList& train_routes,
	const std::string& title,
	DebugLevel::Level level
) {
	auto output_indent = dout(level).indentWithTitle(title);
	uint i = 0;
	for (auto& route : train_routes) {
		auto route_indent = dout(level).indentWithTitle([&](auto&&out){ out << "train " << i; });
		::util::print_route(route,network,dout(level));
		++i;
	}
}

} // end namespace algo
