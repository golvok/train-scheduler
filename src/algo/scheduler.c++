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
	using Route = std::vector<TrackNetwork::NodeID>;

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
	TrackNetwork::NodeID compute_next_vertex(
		const EdgeWantedCapacities& edge_wanted_capacities,
		const TrainsGoToVertex& trains_go_to_vertex,
		TrackNetwork::NodeID curr
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

const size_t NO_TRAIN = -1;

class Scheduler3 {
	using VertexList = std::vector<TrackNetwork::NodeID>;
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

	class TrainData {
		using TNNodeID = TrackNetwork::NodeID;
	public:
		struct SrcInfo {
			SrcInfo(const TNNodeID& dest) : dest(dest) { }
			TNNodeID dest;
		};
		struct DestInfo {
			DestInfo(const TNNodeID& src) : src(src) { }
			TNNodeID src;
		};

		TrainData(VertexList&& train)
			: train(train)
			, src_infos{ {train.front(), SrcInfo(train.back())} }
			, dest_infos{ {train.back(), DestInfo(train.front())} }
		{ }

		void add_src_dest_pair(const TNNodeID& src, const TNNodeID& dest) {
			src_infos.emplace(src, SrcInfo(dest));
			dest_infos.emplace(dest, DestInfo(src));
		}

		bool is_src      (const TNNodeID& node_id) const { return src_infos.find(node_id) != src_infos.end(); }
		auto get_srces   (                       ) const { return ::util::xrange_forward_pe<decltype(src_infos.begin())>(src_infos.begin(), src_infos.end(), [](const auto& it) { return it->first; }); }
		auto get_dests_of(const TNNodeID& node_id) const { auto r = src_infos.equal_range(node_id); return ::util::xrange_forward_pe<decltype(r.first)>(r.first, r.second, [](const auto& e) { return e->second.dest; }); }

		bool is_dest     (const TNNodeID& node_id) const { return dest_infos.find(node_id) != dest_infos.end(); }
		auto get_dests   (                       ) const { return ::util::xrange_forward_pe<decltype(dest_infos.begin())>(dest_infos.begin(), dest_infos.end(), [](const auto& it) { return it->first; }); }
		auto get_srces_of(const TNNodeID& node_id) const { auto r = dest_infos.equal_range(node_id); return ::util::xrange_forward_pe<decltype(r.first)>(r.first, r.second, [](const auto& e) { return e->second.src; }); }

		VertexList& get_train() { return train; }
		const VertexList& get_train() const { return train; }
	private:
		VertexList train;

		std::unordered_multimap<TNNodeID, SrcInfo> src_infos;
		std::unordered_multimap<TNNodeID, DestInfo> dest_infos;
	};

	using TrainDataList = std::vector<TrainData>;

private:
	TrainDataList make_one_train_per_passenger();
	TrainDataList coalesce_trains(TrainDataList&& initial_trains);

	void dump_trains_to_dout(
		const TrainDataList& train_data,
		const std::string& title,
		DebugLevel::Level level
	);
};

Scheduler3::TrainDataList remove_redundant_trains(
	Scheduler3::TrainDataList&& train_data,
	const std::vector<size_t>& train_is_rudundant_with
);

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
				for (TrackNetwork::NodeID id : route_for_p) {
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
		TrackNetwork::NodeID next = compute_next_vertex(
			edge_wanted_capacities,
			trains_go_to_vertex,
			route.back()
		);

		if (next == TrackNetwork::NodeID(-1)) {
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

TrackNetwork::NodeID Scheduler2::compute_next_vertex(
	const EdgeWantedCapacities& edge_wanted_capacities,
	const TrainsGoToVertex& trains_go_to_vertex,
	TrackNetwork::NodeID curr
) {
	auto vertex_index = dout(DL::TR_D2).indentWithTitle([&](auto&&out){ out << "Vertex " << network.getVertexName(curr); });

	// check if all adjacent vertices already have trains -- cache?
	bool heed_trains_going_to = false;
	for (TrackNetwork::NodeID v : make_iterable(adjacent_vertices(curr,g))) {
		if (trains_go_to_vertex[v] == false) {
			heed_trains_going_to = true;
			break;
		}
	}

	if (!heed_trains_going_to) {
		dout(DL::TR_D2) << "-- ignoring existing trains --\n";
	}

	TrackNetwork::NodeID next(-1);
	float best_wanted_capacity = 0;

	for (auto e_desc : make_iterable(out_edges(curr,g))) {
		TrackNetwork::NodeID target_v = target(e_desc,g);
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

	auto train_data = make_one_train_per_passenger();

	dump_trains_to_dout(train_data, "Initial Trains", DL::TR_D1);

	train_data = coalesce_trains(std::move(train_data));

	std::vector<TrainRoute> train_routes;

	for(auto& datum : train_data) {
		auto repeat_time = datum.get_train().size();
		train_routes.emplace_back(
			::util::make_id<::algo::RouteID>(train_routes.size()),
			std::move(datum.get_train()),
			std::vector<TrackNetwork::Time>{ 0 }, // assumes all routes start at time 0
			repeat_time, // make it repeat after it's done for now
			network
		);
	}

	return Schedule("Scheduler3 schedule",std::move(train_routes));
}

Scheduler3::TrainDataList Scheduler3::make_one_train_per_passenger() {
	TrainDataList train_data;
	for (const auto& p : passengers) {
		train_data.emplace_back(::util::get_shortest_route(
			p.getEntryID(),
			p.getExitID(),
			network
		));
		train_data.back().add_src_dest_pair(p.getEntryID(), p.getExitID());
	}
	return std::move(train_data);
}

Scheduler3::TrainDataList Scheduler3::coalesce_trains(TrainDataList&& train_data) {

	size_t old_size = train_data.size();
	int iter_num = 1;
	while (true) {
		auto iter_indent = dout(DL::TR_D2).indentWithTitle([&](auto&& str) {
			str << "Coalescing Iteration " << iter_num;
		});

		// the train that will take over the duties of the i'th train.
		// not sure we need to do this every time...
		std::vector<size_t> train_is_rudundant_with(train_data.size(), NO_TRAIN);

		for (size_t train_i = 0; train_i != train_data.size(); ++train_i) {
			if (train_is_rudundant_with[train_i] != NO_TRAIN) { continue; }
			auto& train = train_data[train_i].get_train();

			for (size_t comp_train_i = train_i + 1; comp_train_i != train_data.size(); ++comp_train_i) {
				if (train_is_rudundant_with[comp_train_i] != NO_TRAIN) { continue; }
				auto& comp_train = train_data[comp_train_i].get_train();

				// TODO: this DOES NOT handle repeated vertices...
				// TODO: keep track of how many (and what wanted capacity) routes get combinded
				//       and where, so that any splitting/moving is aware of the weighting. Store
				//       per edge?
				// IDEA: A "move" is a swap of edges between train_data, or a re-route.
				//       A timing analysis is done after.

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
						// they overlap, and train is longer, eg:
						//              \/ trian ends
						//  ...==>+====>+--->+  <- comp train ends
						redundant_train = comp_train_i;
					}
				} else if (reached_end && !comp_reached_end) {
					if (comp_first_match == comp_train.begin()) {
						// extend train? maybe. So, add to consideration. (TODO)
						continue;
					} else {
						// they overlap, and comp trian is longer, eg:
						//              \/ comp trian ends
						//  ...==>+====>+--->+  <- train ends
						redundant_train = train_i;
					}
				} else {
					// both end on the same vertex, so remove the shorter one.
					// (the one that could have it's start vertex found in the other)
					if (first_match == train.begin()) {
						redundant_train = train_i;
					} else if (comp_first_match == comp_train.begin()) {
						redundant_train = comp_train_i;
					} else {
						::util::print_and_throw<std::runtime_error>([&](auto&& err) { err << "don't handle convergence case\n"; });
					}
				}

				if (redundant_train == (size_t)-1) {
					::util::print_and_throw<std::runtime_error>([&](auto&& err) { err << "no train selected as redundant!\n"; });
				}

				size_t other_train = redundant_train == train_i ? comp_train_i : train_i;
				train_is_rudundant_with[redundant_train] = other_train;

				dout(DL::TR_D3) << "train #" << redundant_train << " marked redundant in favour of #" << other_train << "\n";
			}
		}

		// copy over the src,dest pairs from the redundant train to the replacement train
		for (size_t itrain = 0; itrain != train_data.size(); ++itrain) {
			auto repalcemnt_train = train_is_rudundant_with[itrain];
			if (repalcemnt_train == NO_TRAIN) { continue; }

			for (const auto& src : train_data[itrain].get_srces()) {
				for (const auto& dest : train_data[itrain].get_dests_of(src)) {
					train_data[repalcemnt_train].add_src_dest_pair(src, dest);
				}
			}
		}

		train_data = remove_redundant_trains(std::move(train_data), train_is_rudundant_with);
		train_is_rudundant_with = std::vector<size_t>(train_data.size(), NO_TRAIN);

		dump_trains_to_dout(train_data, "Trains After basic redundancy removal", DL::TR_D2);

		/* pick a route (for each route?), see which (src,dest)s can't make it without this route
		 * then figure out how to fix that, while lowering cost (?)
		 * What is cost though? obviously number of trains, but this is too discrete
		 * Total route lengths? will that end up with fewer trains?
		 *     I feel this will result in many small trains, feeding into a big one.
		 *     Is there a way to make bad moves? and do a simulated annealing thing?
		 *         bad moves would be... duplication? well, the case of adding two small spur routes is "bad"
		 *         hmm... that's fine actually. Would need to have a move that combines 2+ spurs & normal sized one into 3 routes to counter
		 */

		struct SrcDestPair {
			TrackNetwork::NodeID src;
			TrackNetwork::NodeID dest;
		};


		for (size_t itrain = 0; itrain != train_data.size(); ++itrain) {
			std::vector<SrcDestPair> needs_this_train;

			auto check_if_needs_this_train = [&](const auto& test_id, const auto& other_id, bool reverse) {
				const auto find_results = ::util::find_with_index(
					train_data.begin(), train_data.end(),
					[&](const auto& train_datum, const auto& index) {
						const auto& train = train_datum.get_train();
						return
							( index != itrain )
							&& ( train.end() != std::find(train.begin(), train.end(), test_id) )
						;
					}
				);
				const auto other_train_iter = find_results.first;

				if (other_train_iter == train_data.end()) {
					if (reverse) {
						needs_this_train.emplace_back(SrcDestPair{other_id, test_id});
					} else {
						needs_this_train.emplace_back(SrcDestPair{test_id, other_id});
					}
				}
			};

			for (const auto& src : train_data[itrain].get_srces()) {
				for (const auto& dest : train_data[itrain].get_dests_of(src)) {
					check_if_needs_this_train(dest, src, true);
				}
			}

			for (const auto& dest : train_data[itrain].get_dests()) {
				for (const auto& src : train_data[itrain].get_srces_of(dest)) {
					check_if_needs_this_train(src, dest, false);
				}
			}

			if (needs_this_train.size() == 0) {
				train_is_rudundant_with[itrain] = -2; // anything but NO_TRAIN...
				dout(DL::TR_D1) << "no one NEEDS train " << itrain << '\n';
			} else {
				dout(DL::TR_D1) << "someone needs train " << itrain << '\n';
			}
		}

		train_data = remove_redundant_trains(std::move(train_data), train_is_rudundant_with);

		dump_trains_to_dout(train_data, "Trains After No-Need-Removal", DL::TR_D2);

		if (train_data.size() <= max_trains_at_a_time || train_data.size() == old_size || iter_num == 10) {
			break;
		}
		old_size = train_data.size();
		iter_num += 1;
	}

	return std::move(train_data);
}

Scheduler3::TrainDataList remove_redundant_trains(
	Scheduler3::TrainDataList&& train_data,
	const std::vector<size_t>& train_is_rudundant_with
) {
	auto train_is_rudundant_at_this_index = [&](const auto& index) {
		return train_is_rudundant_with[index] != NO_TRAIN;
	};

	train_data.erase(
		::util::remove_by_index(
			train_data.begin(), train_data.end(),
			train_is_rudundant_at_this_index
		),
		train_data.end()
	);

	return train_data;
}

void Scheduler3::dump_trains_to_dout(
	const TrainDataList& train_data,
	const std::string& title,
	DebugLevel::Level level
) {
	auto output_indent = dout(level).indentWithTitle(title);
	uint i = 0;
	for (const auto& datum : train_data) {
		auto route_indent = dout(level).indentWithTitle([&](auto&&out){ out << "train " << i; });
		::util::print_route(datum.get_train(),network,dout(level));
		++i;
	}
}

} // end namespace algo
