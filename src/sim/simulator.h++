
#ifndef SIM__SIMULATOR_HPP
#define SIM__SIMULATOR_HPP

#include <algo/scheduler.h++>
#include <util/handles.h++>
#include <util/track_network.h++>

#include <functional>
#include <memory>

namespace sim {

class Simulator;
class SimulatorHandle;

using SimTime = double;
using SimTimeInterval = std::pair<SimTime,SimTime>;

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

class SimulatorHandle : public ::util::shared_handle<Simulator> {
public:
	using shared_handle::shared_handle;
	using shared_handle::operator=;

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

	void runForTime(const SimTime& time_to_run, const SimTime& step_size);
	SimTime getCurrentTime();

	std::shared_ptr<const ::algo::Schedule> getScheduleUsed();
	std::shared_ptr<const TrackNetwork> getTrackNetworkUsed();

	void registerObserver(ObserverType observer, SimTime period);
	bool isPaused();

private:
	const Train2PositionInfoMap& getTrainLocations() const;

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
