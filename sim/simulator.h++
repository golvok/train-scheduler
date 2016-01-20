
#ifndef SIM__SIMULATOR_HPP
#define SIM__SIMULATOR_HPP

#include <algo/scheduler.h++>
#include <util/track_network.h++>

#include <functional>
#include <memory>

namespace sim {

class Simulator;
class SimulatorHandle;

struct TrainLocation {
	size_t edge_number;
	double fraction_through_edge;

	TrainLocation() : edge_number(0), fraction_through_edge(0) { }
	TrainLocation(size_t edge_number, double fraction_through_edge) : edge_number(edge_number), fraction_through_edge(fraction_through_edge) { }
	TrainLocation(const TrainLocation&) = default;
	TrainLocation(TrainLocation&&) = default;
	TrainLocation& operator=(const TrainLocation&) = default;
	TrainLocation& operator=(TrainLocation&&) = default;
};

using Train2PositionInfoMap = ::algo::TrainMap<TrainLocation>;

using ObserverType = std::function<bool()>;

class SimulatorHandle {
public:
	SimulatorHandle() : sim_ptr() { }

	auto getActiveTrains() const {
		using map_iter = Train2PositionInfoMap::const_iterator;
		const auto& map = getTrainLocations();
		return ::util::make_generator<map_iter>(
			map.begin(),
			map.end(),
			[](const map_iter& it) {
				return std::next(it);
			},
			[](const map_iter& it) {
				return it->first;
			}
		);
	}
	// std::vector<std::reference_wrapper<Passenger>> getActivePassengers() const;

	const TrainLocation& getTrainLocation(const ::algo::TrainID& train) const;
	const PassengerConstRefList& getPassengersAt(const ::algo::TrainID& train) const;
	const PassengerConstRefList& getPassengersAt(const StationID& station) const;

	void runForTime(const TrackNetwork::Time& t);
	TrackNetwork::Time getCurrentTime();

	SimulatorHandle(const SimulatorHandle&) = default;
	SimulatorHandle& operator=(const SimulatorHandle&) = default;
	SimulatorHandle& operator=(SimulatorHandle&&) = default;

	operator bool() { return (bool)sim_ptr; }
	void reset() { sim_ptr.reset(); }

	std::shared_ptr<const ::algo::Schedule> getScheduleUsed();
	std::shared_ptr<const TrackNetwork> getTrackNetworkUsed();

	void registerObserver(ObserverType observer, TrackNetwork::Time period);
	bool isPaused();
private:
	SimulatorHandle(const std::shared_ptr<Simulator>& sim_ptr) : sim_ptr(sim_ptr) { }

	const Train2PositionInfoMap& getTrainLocations() const;

	std::shared_ptr<Simulator> sim_ptr;

	friend SimulatorHandle instantiate_simulator(
		std::shared_ptr<const PassengerList> passengers,
		std::shared_ptr<const ::algo::Schedule> schedule,
		std::shared_ptr<const TrackNetwork> tn
	);
	friend class Simulator;
};

SimulatorHandle instantiate_simulator(
	std::shared_ptr<const PassengerList> passengers,
	std::shared_ptr<const ::algo::Schedule> schedule,
	std::shared_ptr<const TrackNetwork> tn
);

} // end namespace sim

#endif /* SIM__SIMULATOR_HPP */
