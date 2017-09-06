
#include "simulator.h++"

#include <algo/passenger_routing.h++>
#include <util/thread_utils.h++>

#include <list>
#include <mutex>

namespace sim {

class Simulator {
public:
	Simulator(
		const PassengerGeneratorFactory::PassengerGeneratorCollection* passenger_generators,
		std::shared_ptr<const ::algo::Schedule> schedule,
		std::shared_ptr<const TrackNetwork> tn
	)
	: passenger_generators(*passenger_generators)
	, schedule(schedule)
	, tn(tn)
	, observers_and_periods()
	, current_time()
	, passenger_routes()
	, passenger_list()
	, passengers_on_trains()
	, passengers_at_stations(tn->makeStationMap<PassengerIDSet>())
	, train_locations()
	, passenger_paths()
	, passenger_histories()
	, passenger_id_generator(0)
	, is_paused(true)
	, is_paused_mutex()
	, sim_task_controller()
	{ }

	Simulator(const Simulator&) = delete;
	Simulator(Simulator&&) = delete;
	Simulator& operator=(const Simulator&) = delete;
	Simulator& operator=(Simulator&&) = delete;

	~Simulator();

	const Train2PositionInfoMap& getTrainLocations() const;
	const TrainLocation& getTrainLocation(const ::algo::TrainID& train) const;

	const PassengerList& getPassengerList() const;
	const PassengerIDSet& getPassengerIDsAt(const ::algo::TrainID& train) const;
	const PassengerIDSet& getPassengerIDsAt(const StationID& station) const;

	void runForTime(const SimTime& time_to_run, const SimTime& max_step_size);
	SimTime advanceUntilEvent(const SimTime& sim_until_time);
	SimTime getCurrentTime() { return current_time; }

	void movePassengerFromHereGoingTo(
		const PassengerID& passenger,
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

	const auto& getPassengerHistories() const { return passenger_histories; }

	// to be move to a CachingPassengerRouter (or something)
	const algo::PassengerRoutes::RouteType& getRouteFor(PassengerID pid);
	const algo::PassengerRoutes::RouteType& getRouteFor(const Passenger& p);

private:
	const PassengerGeneratorFactory::PassengerGeneratorCollection& passenger_generators;
	std::shared_ptr<const ::algo::Schedule> schedule;
	std::shared_ptr<const TrackNetwork> tn;

	std::list<std::pair<ObserverType, SimTime>> observers_and_periods;

	SimTime current_time;

	::algo::PassengerRoutes passenger_routes; // to ba part of CachingPassengerRouter

	PassengerList passenger_list;
	::algo::TrainMap<PassengerIDSet> passengers_on_trains;
	StationMap<PassengerIDSet> passengers_at_stations;

	Train2PositionInfoMap train_locations;

	std::unordered_map<PassengerID, algo::PassengerRoutes::RouteType> passenger_paths;
	struct PassengerExitInfo {
		SimTime time_of_exit;
		algo::PassengerRoutes::RouteType route;

		auto& getRoute() const { return route; }
		auto& getRoute()       { return route; }
	};
	std::unordered_map<PassengerID, PassengerExitInfo> passenger_histories;
	util::IDGenerator<PassengerID> passenger_id_generator;

	bool is_paused;
	std::recursive_mutex is_paused_mutex;

	::util::TaskController sim_task_controller;
};

} // end namespace sim
