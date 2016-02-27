
#include "simulator.h++"

#include <algo/passenger_routing.h++>
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
	, passenger_exits()
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

	void runForTime(const SimTime& time_to_run, const SimTime& max_step_size);
	SimTime advanceUntilEvent(const SimTime& sim_until_time);
	SimTime getCurrentTime() { return current_time; }

	void movePassengerFromHereGoingTo(
		const Passenger& passenger,
		const LocationID& from_location,
		const LocationID& to_location,
		const SimTime& time_of_move
	);

	const std::shared_ptr<const ::algo::Schedule> getScheduleUsed() const { return schedule; }
	const std::shared_ptr<const TrackNetwork> getTrackNetworkUsed() const { return tn; }

	void registerObserver(ObserverType observer, SimTime period);
	bool isPaused() { std::unique_lock<std::recursive_mutex> paused_ul(is_paused_mutex); return is_paused; }
	void setIsPaused(bool val) { std::unique_lock<std::recursive_mutex> paused_ul(is_paused_mutex); is_paused = val; }

	// internal methods

	const auto& getPassengerExits() const { return passenger_exits; }
private:
	std::shared_ptr<const PassengerList> passengers;
	std::shared_ptr<const ::algo::Schedule> schedule;
	std::shared_ptr<const TrackNetwork> tn;

	std::list<std::pair<ObserverType, SimTime>> observers_and_periods;

	SimTime current_time;

	::algo::PassengerRoutes passenger_rotues;

	::algo::TrainMap<PassengerConstRefList> passengers_on_trains;
	StationMap<PassengerConstRefList> passengers_at_stations;

	Train2PositionInfoMap train_locations;

	struct PassengerExitInfo {
		std::reference_wrapper<const Passenger> passenger;
		SimTime time_of_exit;
	};
	std::vector<PassengerExitInfo> passenger_exits;


	bool is_paused;
	std::recursive_mutex is_paused_mutex;

	::util::TaskController sim_task_controller;
};

} // end namespace sim
