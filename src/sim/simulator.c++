
#include "simulator_internal.h++"

#include <util/logging.h++>

namespace sim {

const TrainLocation& SimulatorHandle::getTrainLocation(const ::algo::TrainID& train) const { return get()->getTrainLocation(train ); }
const PassengerIDSet& SimulatorHandle::getPassengerIDsAt(const ::algo::TrainID& train) const { return get()->getPassengerIDsAt(train  ); }
const PassengerIDSet& SimulatorHandle::getPassengerIDsAt(const StationID& station    ) const { return get()->getPassengerIDsAt(station); }

const Train2PositionInfoMap& SimulatorHandle::getTrainLocations() const { return get()->getTrainLocations(); }
const PassengerList& SimulatorHandle::getPassengerList() const { return get()->getPassengerList(); }

void SimulatorHandle::runForTime(const SimTime& time_to_run, const SimTime& max_step_size) { get()->runForTime(time_to_run, max_step_size); }
SimTime SimulatorHandle::getCurrentTime() { return get()->getCurrentTime(); }


std::shared_ptr<const ::algo::Schedule> SimulatorHandle::getScheduleUsed() { return get()->getScheduleUsed(); }
std::shared_ptr<const TrackNetwork> SimulatorHandle::getTrackNetworkUsed() { return get()->getTrackNetworkUsed(); }

void SimulatorHandle::registerObserver(ObserverType observer, SimTime period) { get()->registerObserver(observer, period); }
bool SimulatorHandle::isPaused() { return get()->isPaused(); }

SimulatorHandle instantiate_simulator(
	const PassengerGeneratorFactory::PassengerGeneratorCollection* passenger_generators,
	std::shared_ptr<const ::algo::Schedule> schedule,
	std::shared_ptr<const TrackNetwork> tn
) {
	return SimulatorHandle(std::make_shared<Simulator>(passenger_generators, schedule, tn));
}

Simulator::~Simulator() {
	dout(DL::SIM_D1) << "destroying simulator\n";
}

const Train2PositionInfoMap& Simulator::getTrainLocations() const {
	return train_locations;
}

const TrainLocation& Simulator::getTrainLocation(const ::algo::TrainID& train) const {
	const auto find_results = this->train_locations.find(train);

	if (find_results == this->train_locations.end()) {
		::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
			str << "train " << train << " is not active. t = " << current_time << "\n";
		});
	}

	return find_results->second;
}

const PassengerList& Simulator::getPassengerList() const { return passenger_list; }

const PassengerIDSet& Simulator::getPassengerIDsAt(const ::algo::TrainID& train) const {
	return passengers_on_trains.at(train);
}

const PassengerIDSet& Simulator::getPassengerIDsAt(const StationID& station) const {
	return passengers_at_stations.at(station.getValue());
}

void Simulator::runForTime(const SimTime& time_to_run, const SimTime& max_step_size) {
	const auto job_token = sim_task_controller.getJobToken();
	if (sim_task_controller.isCancelRequested()) { return; }

	const auto stop_time = current_time + time_to_run;

	auto last_observer_update_time = current_time;

	while (true) {
		if (sim_task_controller.isCancelRequested()) { return; }

		const auto time_left = stop_time - current_time;

		if (time_left <= 0) {
			break;
		}

		const auto next_step_time = current_time + std::min(time_left, max_step_size);
		const auto next_observer_update_time = (
			observers_and_periods.empty()
		) ? (
			// if no observers, just use max
			std::numeric_limits<SimTime>::max()
		) : (
			// else, try to advance by shortest observer period (list is sorted)
			last_observer_update_time + observers_and_periods.front().second
		);

		enum class WhichTime {
			STEP, OBSERVER,
		};
		const auto sim_until_time_compare_results = std::min(
			compare_with_tag(next_step_time,            WhichTime::STEP    ),
			compare_with_tag(next_observer_update_time, WhichTime::OBSERVER)
		);
		const auto sim_until_time = sim_until_time_compare_results.value();

		setIsPaused(false);

		auto time_advavced = advanceUntilEvent(sim_until_time);
		(void)time_advavced;

		setIsPaused(true);

		if (current_time > sim_until_time) {
			dout(DL::WARN) << "simulated too much! wanted " << sim_until_time << " but got " << current_time << '\n';
		}

		// call the observers
		if (sim_until_time_compare_results.id() == WhichTime::OBSERVER) {
			last_observer_update_time = current_time;
			dout(DL::SIM_D1) << "calling the " << observers_and_periods.size() << " observers\n";

			for (auto it = observers_and_periods.begin(); it != observers_and_periods.end();) {

				bool result = it->first(); // execute observer

				// 'it' will get invalidated by the erase, so increment past it first
				const auto it_copy = it;
				++it;

				if (result == false) {
					// remove if return false
					observers_and_periods.erase(it_copy);
				}
			}

		}
	}
}

SimTime Simulator::advanceUntilEvent(const SimTime& sim_until_time) {
	dout(DL::SIM_D2) << "Simulating t:" << current_time << " -> " << sim_until_time << '\n';

	if (sim_until_time < current_time) {
		::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
			str << "trying to simulate backwards! t1=" << current_time << ", t2=" << sim_until_time << '\n';
		});
	}

	// remove passengers that are at the destinations
	for (const auto& station_id : tn->getStaitonRange()) {
		auto& p_list = passengers_at_stations.at(station_id.getValue());

		util::remove_if_assoc(p_list, [&](const auto& pid) {
			const auto& p = passenger_list.at(pid);
			if (getRouteFor(p).back().getLocation() == station_id) {
				dout(DL::SIM_D3) << "passenger " << p.getName() << " left the system at it's destination, " << station_id << '\n';
				return true;
			} else {
				return false;
			}
		});
	}

	// add new trains
	for (const auto& route : schedule->getTrainRoutes()) {
		// find trains starting in t_interval, add them
		for (const auto& train_and_arrival : route.getTrainsLeavingInInterval({current_time, sim_until_time + 1}, *tn)) {
			if (train_and_arrival.train.getDepartureTime() < current_time || train_and_arrival.train.getDepartureTime() >= sim_until_time) {
				dout(DL::SIM_D3) << "skipping adding train " << train_and_arrival.train << ", departs at t=" << train_and_arrival.train.getDepartureTime() << ", arrival t=" << train_and_arrival.arrival << '\n';
				continue;
			}
			dout(DL::SIM_D3) << "adding train " << train_and_arrival.train << ", departs at t=" << train_and_arrival.train.getDepartureTime() << '\n';
			train_locations.emplace(train_and_arrival.train.getTrainID(), TrainLocation());
		}
	}

	// delete me:
	// for (const auto& p : *passengers) {
	// 	const auto& route = passenger_routes.getRoute(p);
	// 	const auto& first_re = route.front();
	// 	if (current_time <= first_re.getTime() && first_re.getTime() < sim_until_time) {
	// 		dout(DL::SIM_D3) << "adding passenger " << p << '\n';
	// 		passengerRefListAdd(passengers_at_stations.at(first_re.getLocation().asStationID().getValue()),p);
	// 	}
	// }

	// inject passengers
	for (const auto& p_gen : passenger_generators) {
		for (auto&& p : p_gen.leavingDuringInterval(current_time, sim_until_time, passenger_id_generator)) {
			dout(DL::SIM_D3) << "adding passenger " << p << '\n';
			const auto pid = p.getID();
			passenger_list.emplace(pid,std::move(p));
			passengers_at_stations.at(p.getEntryID()).emplace(pid);
			passenger_paths[pid].emplace_back(getRouteFor(p).front()); // add start RE
		}
	}

	std::vector<::algo::TrainID> trains_to_remove;

	// move trains & passengers
	for (auto& value_pair : train_locations) {
		// move trains to where they should be

		const auto& trainID = value_pair.first;
		const auto& routeID = trainID.getRouteID();
		const auto& route = schedule->getTrainRoute(routeID);
		const auto& train = route.makeTrainFromIndex(trainID.getTrainIndex());
		auto& position_info = value_pair.second;

		SimTime time_until_prev_vertex = 0;

		const auto train_indent = dout(DL::SIM_D3).indentWithTitle([&](auto&& str) {
			str << "Updating Train " << train;
		});

		// handle trains that are starting in the simulated interval
		const auto time_to_departure_from_interval_start = train.getDepartureTime() - current_time;

		if (sim_until_time < time_to_departure_from_interval_start) {
			::util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "train " << train << " departs in the future!\n";
			});
		}

		if (
			position_info.edge_number == 0 &&
			position_info.fraction_through_edge == 0 &&
			time_to_departure_from_interval_start < 0
		) {
			::util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "train " << train << " departed in the past!\n";
			});
		}

		// create a constant subtraction, if this train starts in the future
		const auto departing_subtraction = time_to_departure_from_interval_start > 0 ? time_to_departure_from_interval_start : 0;

		// figure out which edge the train should be on, and update position_info
		auto prev_vertex_it = route.getPath().begin() + position_info.edge_number;
		auto next_vertex_it = prev_vertex_it + 1;

		while (true) {
			dout(DL::SIM_D3) << "current position: e#=" << position_info.edge_number << ", fte=" << position_info.fraction_through_edge << '\n';

			const auto current_time_in_simulation_of_this_train =
				current_time + time_until_prev_vertex - departing_subtraction
			;

			if (position_info.fraction_through_edge == 0) {
				const auto& arriving_station_id = tn->getStationIDByVertexID(*prev_vertex_it);

				// make copies, so that if the loops modify (which they do) then nothing confusing happens
				// this isn't very efficient, but it's the most straightforward...
				const auto old_passengers_at_the_station = passengers_at_stations.at(arriving_station_id.getValue());
				const auto old_passengers_on_this_train = passengers_on_trains[trainID];

				// pickup passengers
				for (const auto& p : old_passengers_at_the_station) {
					movePassengerFromHereGoingTo(p, arriving_station_id, trainID, current_time_in_simulation_of_this_train);
				}

				// and drop off passengers
				for (const auto& p : old_passengers_on_this_train) {
					movePassengerFromHereGoingTo(p, trainID, arriving_station_id, current_time_in_simulation_of_this_train);
				}
			}

			if (next_vertex_it == route.getPath().end()) {
				// ie. if the "prev" vertex is the last one.
				if (passengers_on_trains[trainID].empty() == false) {
					::util::print_and_throw<std::runtime_error>([&](auto&& str) {
						str << " train " << train << " had the passengers ";
						util::print_container(passengers_on_trains[trainID], str);
						str << " when it exited!\n";
					});
				}
				dout(DL::SIM_D3) << "destination reached\n";
				trains_to_remove.push_back(trainID);
				break;
			}

			const auto time_left_to_simulate =
				sim_until_time - current_time_in_simulation_of_this_train;
			;

			if (time_left_to_simulate <= 0) {
				break;
			}

			const auto fraction_left_to_travel = 1 - position_info.fraction_through_edge;

			using std::next;
			// calculate the time it would take to get to the next vertex
			const auto additional_time_to_next_vertex = (
				(
					fraction_left_to_travel
				) * (
					train.getExpectedTravelTime(
						{prev_vertex_it, next(next_vertex_it)}, *tn
					)
				)
			);

			// the next vertex would be too far, so it's currently on this edge
			if (additional_time_to_next_vertex > time_left_to_simulate) {
				position_info.fraction_through_edge += (
					(
						fraction_left_to_travel
					) * (
						time_left_to_simulate
					) / (
						additional_time_to_next_vertex
					)
				);
				if (!(0 <= position_info.fraction_through_edge && position_info.fraction_through_edge <= 1)) {
					::util::print_and_throw<std::runtime_error>([&](auto&& str) {
						str << "train " << train << ": setting fte to " << position_info.fraction_through_edge << '\n';
					});
				}
				break;
			}

			// if we get here, the train will be just about to arrive at the "next" station
			dout(DL::SIM_D2) << "train " << train << " just about to arrive at " << tn->getVertexName(*next_vertex_it) << '\n';

			// "just arrive" at station, by making previous what the next was
			prev_vertex_it = next_vertex_it;
			++next_vertex_it;
			++position_info.edge_number;
			position_info.fraction_through_edge = 0;
			time_until_prev_vertex += additional_time_to_next_vertex;
		}
	}

	// remove marked trains
	for (const auto& tid : trains_to_remove) {
		train_locations.erase(tid);
	}

	// update time
	current_time = sim_until_time;

	return sim_until_time - current_time;
}

/**
 * If passenger wants to be moved from from_location to to_location, then do so.
 * If that is not it's next move, then don't do anything.
 */
void Simulator::movePassengerFromHereGoingTo(
	const PassengerID& passenger_id,
	const LocationID& from_location,
	const LocationID& to_location,
	const SimTime& time_of_move
) {
	const auto route = getRouteFor(passenger_id);

	// find the next route element
	const auto next_route_element_it = [&]() {
		auto prev = begin(route);
		if (prev == end(route)) { return prev; }
		auto curr = next(prev);
		while (curr != end(route)) {
			if (prev->getLocation() == from_location && curr->getLocation() == to_location) {
				return curr;
			}
			prev = curr;
			curr = next(prev);
		}
		return curr;
	}();

	if (next_route_element_it == route.begin()) {
		// if (current_location != from_location) {
		// 	// ignore
		// 	return;
		// } else {
			// this passenger hasn't started yet
			::util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "passenger " << passenger_list.at(passenger_id).getName() << " hasn't entered the system (or has empty route)!\n";
			});
		// }
	}

	const auto& current_route_element = *std::prev(next_route_element_it); // note: route can't be empty
	const auto& current_location = current_route_element.getLocation();

	if (next_route_element_it == route.end()) {
		// this passenger has exited the system
		// if (from_location.isTrain()) {
		// 	::util::print_and_throw<std::runtime_error>([&](auto&& str) {
		// 		str << "passenger " << passenger_list.at(passenger_id).getName() << " doesn't want to (ever) get off!\n";
		// 	});
		// } else {
			// ignore
			return;
		// }
	}

	const auto& next_location = next_route_element_it->getLocation(); // note: next is not end()

	if (current_location != from_location) {
		::util::print_and_throw<std::runtime_error>([&](auto&& str) {
			str << "passenger " << passenger_list.at(passenger_id).getName() << " (at " << std::tie(from_location,*tn) << ") not at location expected (" << std::tie(current_location,*tn) << ")!\n";
		});
		return; // do nothing
	}

	(void)to_location;
	if (next_location != to_location) {
		// ::util::print_and_throw<std::runtime_error>([&](auto&& str) {
		// 	str << "passenger " << passenger_list.at(passenger_id).getName() << " (at " << std::tie(from_location,*tn) << ") not going to location expected (" << std::tie(to_location,*tn) << ")!\n";
		// });
		dout(DL::SIM_D3) << "passenger " << passenger_list.at(passenger_id).getName() << " (at " << std::tie(from_location,*tn) << ") doesn't want to go to " << std::tie(to_location,*tn) << "\n"; 
		return; // ignore
	}
	dout(DL::SIM_D3) << "passenger " << passenger_list.at(passenger_id).getName() << " : " << std::tie(current_location,*tn) << " -> " << std::tie(next_location,*tn) << '\n';

	if (current_location.isStation()) {
		if (next_location.isTrain()) {
			passengers_at_stations.at(current_location.asStationID().getValue()).erase(passenger_id);
			passengers_on_trains[next_location.asTrainID()].insert(passenger_id);
		} else {
			::util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "passenger going to unexpected location type: S->not T !\n";
			});
		}
	} else if (current_location.isTrain()) {
		if (next_location.isStation()) {
			passengers_on_trains[current_location.asTrainID()].erase(passenger_id);
			passengers_at_stations.at(next_location.asStationID().getValue()).insert(passenger_id);
		} else {
			::util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "passenger going to unexpected location type: T->not S!\n";
			});
		}
	} else {
		::util::print_and_throw<std::runtime_error>([&](auto&& str) {
			str << "passenger at unexpected location type!\n";
		});
	}

	passenger_paths[passenger_id].emplace_back(to_location, time_of_move);

	if (next_location == tn->getStationIDByVertexID(passenger_list.at(passenger_id).getExitID())) {
		// mark as exited
		passenger_histories.emplace(passenger_id, PassengerExitInfo{time_of_move, passenger_paths.at(passenger_id)});
	}
}

void Simulator::registerObserver(ObserverType observer, SimTime period) {
	const auto prev_iter = std::find_if(observers_and_periods.begin(), observers_and_periods.end(), [&](const auto& elem) {
		return elem.second < period;
	});
	observers_and_periods.emplace(prev_iter, std::make_pair(observer, period));
	dout(DL::SIM_D1) << "added observer, period=" << period << '\n';
}

const algo::PassengerRoutes::RouteType& Simulator::getRouteFor(PassengerID pid) {
	return getRouteFor(passenger_list.at(pid));
}

const algo::PassengerRoutes::RouteType& Simulator::getRouteFor(const Passenger& p) {
	if (passenger_routes.hasRoute(p) == false) {
		const auto& indent = dout(DL::SIM_D3).indentWithTitle([&](auto&& str) {
			str << "re-routing passenger " << p;
		});
		const auto& [route, ch] = algo::route_through_schedule(
			*tn,
			*schedule,
			p.getStartTime(),
			p.getEntryID(),
			p.getExitID()
		);
		passenger_routes.addRoute(p, route);
		(void)ch; // TODO do something with the cache handle
	}
	return passenger_routes.getRoute(p);
}

} // end namespace sim
