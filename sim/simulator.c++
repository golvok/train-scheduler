
#include "simulator.h++"

#include <algo/passenger_routing.h++>
#include <util/logging.h++>
#include <util/thread_utils.h++>

#include <list>
#include <mutex>

namespace sim {

class Simulator {
public:
	Simulator(
		std::shared_ptr<const PassengerList> passengers,
		std::shared_ptr<const ::algo::Schedule> schedule,
		std::shared_ptr<const TrackNetwork> tn
	)
	: passengers(passengers)
	, schedule(schedule)
	, tn(tn)
	, observers_and_periods()
	, current_time()
	, passenger_rotues(::algo::route_passengers(*tn, *schedule, *passengers))
	, passengers_on_trains(schedule->makeTrainMap<PassengerConstRefList>())
	, passengers_at_stations(tn->makeStationMap<PassengerConstRefList>())
	, train_locations()
	, is_paused(true)
	, is_paused_mutex()
	, sim_task_controller()
	{ }

	Simulator(const Simulator&) = delete;
	Simulator(Simulator&&) = delete;
	Simulator& operator=(const Simulator&) = delete;
	Simulator& operator=(Simulator&&) = delete;

	~Simulator();

	// std::vector<std::reference_wrapper<Passenger>> getActivePassengers() const;

	const TrainLocation& getTrainLocation(const ::algo::TrainID& train) const;
	const Train2PositionInfoMap& getTrainLocations() const;
	const PassengerConstRefList& getPassengersAt(const ::algo::TrainID& train) const;
	const PassengerConstRefList& getPassengersAt(const StationID& station) const;

	void runForTime(const TrackNetwork::Time& t);
	void advanceBy(const TrackNetwork::Time& t);
	TrackNetwork::Time getCurrentTime() { return current_time; }

	void movePassengerFromHereGoingTo(
		const Passenger& passenger,
		const LocationID& from_location,
		const LocationID& to_location
	);

	const std::shared_ptr<const ::algo::Schedule> getScheduleUsed() const { return schedule; }
	const std::shared_ptr<const TrackNetwork> getTrackNetworkUsed() const { return tn; }

	void registerObserver(ObserverType observer, TrackNetwork::Time period);
	bool isPaused() { std::unique_lock<std::recursive_mutex> paused_ul(is_paused_mutex); return is_paused; }
	void setIsPaused(bool val) { std::unique_lock<std::recursive_mutex> paused_ul(is_paused_mutex); is_paused = val; }

private:
	std::shared_ptr<const PassengerList> passengers;
	std::shared_ptr<const ::algo::Schedule> schedule;
	std::shared_ptr<const TrackNetwork> tn;

	std::list<std::pair<ObserverType, TrackNetwork::Time>> observers_and_periods;

	TrackNetwork::Time current_time;

	::algo::PassengerRoutes passenger_rotues;

	::algo::TrainMap<PassengerConstRefList> passengers_on_trains;
	StationMap<PassengerConstRefList> passengers_at_stations;

	Train2PositionInfoMap train_locations;


	bool is_paused;
	std::recursive_mutex is_paused_mutex;

	::util::TaskController sim_task_controller;
};

// std::vector<std::reference_wrapper<Passenger>> SimulatorHandle::getActivePassengers() const { return sim_ptr->getActivePassengers(); }

const TrainLocation& SimulatorHandle::getTrainLocation(const ::algo::TrainID& train) const { return sim_ptr->getTrainLocation(train ); }
const Train2PositionInfoMap& SimulatorHandle::getTrainLocations() const { return sim_ptr->getTrainLocations(); }
const PassengerConstRefList& SimulatorHandle::getPassengersAt (const ::algo::TrainID& train) const { return sim_ptr->getPassengersAt(train  ); }
const PassengerConstRefList& SimulatorHandle::getPassengersAt (const StationID& station  ) const { return sim_ptr->getPassengersAt(station); }

void SimulatorHandle::runForTime(const TrackNetwork::Time& t) { sim_ptr->runForTime(t); }
TrackNetwork::Time SimulatorHandle::getCurrentTime() { return sim_ptr->getCurrentTime(); }


std::shared_ptr<const ::algo::Schedule> SimulatorHandle::getScheduleUsed() { return sim_ptr->getScheduleUsed(); }
std::shared_ptr<const TrackNetwork> SimulatorHandle::getTrackNetworkUsed() { return sim_ptr->getTrackNetworkUsed(); }

void SimulatorHandle::registerObserver(ObserverType observer, TrackNetwork::Time period) { sim_ptr->registerObserver(observer, period); }
bool SimulatorHandle::isPaused() { return sim_ptr->isPaused(); }

SimulatorHandle instantiate_simulator(
	std::shared_ptr<const PassengerList> passengers,
	std::shared_ptr<const ::algo::Schedule> schedule,
	std::shared_ptr<const TrackNetwork> tn
) {
	return SimulatorHandle(std::make_shared<Simulator>(passengers, schedule, tn));
}

Simulator::~Simulator() {
	dout(DL::SIM_D1) << "destroying simulator\n";
}

const Train2PositionInfoMap& Simulator::getTrainLocations() const {
	return train_locations;
}

// std::vector<std:reference_wrapper<Passenger>> Simulator::getActivePassengers() const {
// 	::util::print_and_throw<std::runtime_error>([&](auto&& str) { str << "unimplemented"; });
// 	return {};
// }
const TrainLocation& Simulator::getTrainLocation(const ::algo::TrainID& train) const {
	const auto find_results = this->train_locations.find(train);

	if (find_results == this->train_locations.end()) {
		::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
			str << "train " << train << " is not active. t = " << current_time << "\n";
		});
	}

	return find_results->second;
}

const PassengerConstRefList& Simulator::getPassengersAt(const ::algo::TrainID& train) const {
	return passengers_on_trains.find(train)->second;
}

const PassengerConstRefList& Simulator::getPassengersAt(const StationID& station) const {
	return passengers_at_stations[station.getValue()];
}

void Simulator::runForTime(const TrackNetwork::Time& time_to_run) {
	auto job_token = sim_task_controller.getJobToken();
	if (sim_task_controller.isCancelRequested()) { return; }

	auto stop_time = current_time + time_to_run;

	while (true) {
		if (sim_task_controller.isCancelRequested()) { return; }

		auto time_left = stop_time - current_time;

		if (time_left <= 0) {
			break;
		}

		setIsPaused(false);

		advanceBy(
			(
				observers_and_periods.empty()
			) ? (
				// if no observers, just use the time left
				time_left
			) : (
				// else, advance by shortest observer period (list is sorted)
				// or the time left if that is smaller
				std::min(observers_and_periods.front().second, time_left)
			)
		);

		setIsPaused(true);

		// call the observers
		for (auto it = observers_and_periods.begin(); it != observers_and_periods.end();) {

			bool result = it->first(); // execute observer

			// 'it' will get invalidated by the erase, so increment past it first
			auto it_copy = it;
			++it;

			if (result == false) {
				// remove if return false
				observers_and_periods.erase(it_copy);
			}
		}
	}
}

void Simulator::advanceBy(const TrackNetwork::Time& time_to_simulate) {
	dout(DL::SIM_D2) << "advance by " << time_to_simulate << '\n';

	// if (time_to_simulate == 0) {
	// 	return;
	// }

	if (time_to_simulate < 0) {
		::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
			str << "negative simulation time not allowed: t=" << time_to_simulate << '\n';
		});
	}

	auto t_interval = TrackNetwork::TimeInterval(current_time, current_time + time_to_simulate);

	dout(DL::SIM_D2) << "Simulating t:" << t_interval.first << " -> " << t_interval.second << '\n';

	// remove passengers that are at the destinations
	for (const auto& station_id : tn->getStaitonRange()) {
		auto& p_list = passengers_at_stations[station_id.getValue()];

		p_list.erase(
			std::remove_if(p_list.begin(), p_list.end(), [&](const auto& p) {
				if (passenger_rotues.getRoute(p).back().getLocation() == station_id) {
					dout(DL::SIM_D3) << "passenger " << p.get().getName() << " left the system at it's destination, " << station_id << '\n';
					return true;
				} else {
					return false;
				}
			}),
			p_list.end()
		);
	}

	// add new trains
	for (const auto& route : schedule->getTrainRoutes()) {
		// find trains starting in t_interval, add them
		for (const auto& train : route.getTrainsLeavingInInterval(t_interval, *tn)) {
			dout(DL::SIM_D3) << "adding train " << train << ", departs at t=" << train.getDepartureTime() << '\n';
			train_locations.emplace(train.getTrainID(), TrainLocation());
			passengers_on_trains.emplace(train.getTrainID(), PassengerConstRefList());
		}
	}

	// inject passengers
	for (const auto& p : *passengers) {
		const auto& route = passenger_rotues.getRoute(p);
		const auto& first_re = route.front();
		if (t_interval.first <= first_re.getTime() && first_re.getTime() < t_interval.second) {
			dout(DL::SIM_D3) << "adding passenger " << p << '\n';
			passengerRefListAdd(passengers_at_stations[first_re.getLocation().asStationID().getValue()],p);
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

		TrackNetwork::Time time_until_prev_vertex = 0;

		auto train_indent = dout(DL::SIM_D3).indentWithTitle([&](auto&& str) {
			str << "Updating Train " << train;
		});

		// handle trains that are starting in the simulated interval
		const auto time_to_departure_from_interval_start = train.getDepartureTime() - t_interval.first;

		if (t_interval.second < time_to_departure_from_interval_start) {
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
			dout(DL::SIM_D3) << "current position: en=" << position_info.edge_number << ", fte=" << position_info.fraction_through_edge << '\n';

			if (position_info.fraction_through_edge == 0) {
				const auto& arriving_station_id = tn->getStationIDByVertexID(*prev_vertex_it);

				// make copies, so that if the loops modify (which they do) then nothing confusing happens
				// this isn't very efficient, but it's the most straightforward...
				auto old_passengers_at_the_station = passengers_at_stations[arriving_station_id.getValue()];
				auto old_passengers_on_this_train = passengers_on_trains[train.getTrainID()];

				// pickup passengers
				for (const auto& p : old_passengers_at_the_station) {
					movePassengerFromHereGoingTo(p, arriving_station_id, train.getTrainID());
				}

				// and drop off passengers
				for (const auto& p : old_passengers_on_this_train) {
					movePassengerFromHereGoingTo(p, train.getTrainID(), arriving_station_id);
				}
			}

			if (next_vertex_it == route.getPath().end()) {
				// ie. if the "prev" vertex is the last one.
				if (passengers_on_trains[trainID].empty() == false) {
					::util::print_and_throw<std::runtime_error>([&](auto&& str) {
						str << " train " << train << " had passengers when it exited!\n";
					});
				}
				dout(DL::SIM_D3) << "destination reached\n";
				trains_to_remove.push_back(train.getTrainID());
				break;
			}

			// total time, minus time already covered, minus time to departure if relevant
			const auto time_left_to_simulate =
				time_to_simulate - time_until_prev_vertex - departing_subtraction
			;

			if (time_left_to_simulate <= 0) {
				break;
			}

			const auto fraction_left_to_travel = 1 - position_info.fraction_through_edge;

			// calculate the time it would take to get to the next vertex
			const auto additional_time_to_next_vertex = (
				(
					fraction_left_to_travel
				) * (
					train.getExpectedTravelTime(
						std::make_pair(*prev_vertex_it, *next_vertex_it), *tn
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
			dout(DL::SIM_D2) << "train " << train << " just about to arrive at " << tn->getStationIDByVertexID(*next_vertex_it) << '\n';

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
	current_time = t_interval.second;
}

void Simulator::movePassengerFromHereGoingTo(
	const Passenger& passenger,
	const LocationID& from_location,
	const LocationID& to_location
) {
	const auto& route = passenger_rotues.getRoute(passenger);

	// find the next route element
	const auto next_route_element_it = std::find_if(route.begin(), route.end(), [&](const auto& re) {
		return re.getLocation() == to_location;
	});

	const auto& current_route_element = *std::prev(next_route_element_it); // note: route can't be empty
	const auto& current_location = current_route_element.getLocation();

	if (next_route_element_it == route.begin()) {
		if (current_location != from_location) {
			// ignore
			return;
		} else {
			// this passenger hasn't started yet
			::util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "passenger " << passenger.getName() << " hasn't entered the system (or has empty route)!\n";
			});
		}
	}

	if (next_route_element_it == route.end()) {
		// this passenger has exited the system
		if (current_location.isTrain()) {
			::util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "passenger " << passenger.getName() << " doesn't want to (ever) get off!\n";
			});
		} else {
			// ignore
			return;
		}
	}

	const auto& next_location = next_route_element_it->getLocation(); // note: next is not end()

	if (current_location != from_location) {
		::util::print_and_throw<std::runtime_error>([&](auto&& str) {
			str << "passenger " << passenger.getName() << " not at location expected!\n";
		});
		return; // do nothing
	}

	dout(DL::SIM_D3) << "passenger " << passenger.getName() << " : " << current_location << " -> " << next_location << '\n';

	if (current_location.isStation()) {
		if (next_location.isTrain()) {
			passengerRefListRemove(passengers_at_stations[current_location.asStationID().getValue()], passenger);
			passengerRefListAdd(passengers_on_trains[next_location.asTrainID()], passenger);
		} else {
			::util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "passenger going to unexpected location type: S->not T !\n";
			});
		}
	} else if (current_location.isTrain()) {
		if (next_location.isStation()) {
			passengerRefListRemove(passengers_on_trains[current_location.asTrainID()], passenger);
			passengerRefListAdd(passengers_at_stations[next_location.asStationID().getValue()], passenger);
		} else {
			::util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "passenger going to unexpected location typ: T->not S!\n";
			});
		}
	} else {
		::util::print_and_throw<std::runtime_error>([&](auto&& str) {
			str << "passenger at unexpected location type!\n";
		});
	}
}

void Simulator::registerObserver(ObserverType observer, TrackNetwork::Time period) {
	auto prev_iter = std::find_if(observers_and_periods.begin(), observers_and_periods.end(), [&](auto& elem) {
		return elem.second < period;
	});
	observers_and_periods.emplace(prev_iter, std::make_pair(observer, period));
	dout(DL::SIM_D1) << "added observer, period=" << period << '\n';
}

} // end namespace sim
