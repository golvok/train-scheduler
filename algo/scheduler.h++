#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <util/passenger.h++>
#include <util/track_network.h++>
#include <util/utils.h++>

namespace algo {

struct TrainIDTagType {
	const static uint DEFAULT_VALUE = -1;
};
using TrainID = ::util::ID<uint,TrainIDTagType>;

class Train {
public:
	using Speed = float;

	Train(
		const TrainID train_id,
		std::vector<TrackNetwork::ID>&& route_,
		uint departure_time
	)
		: train_id(train_id)
		, route(std::move(route_))
		, departure_time(departure_time)
		, speed(0.5)
	{ }

	Train(const Train&) = default;
	Train(Train&&) = default;
	Train& operator=(const Train&) = default;
	Train& operator=(Train&&) = default;

	// getters
	TrainID getID() const { return train_id; }
	auto& getRoute() { return route; }
	const auto& getRoute() const { return route; }
	TrackNetwork::ID getEntryID() const { return route.front(); }
	TrackNetwork::ID getExitID() const { return route.back(); }
	uint getDepartureTime() const { return departure_time; }
	Speed getSpeed() const { return speed; }

private:
	TrainID train_id;
	std::vector<TrackNetwork::ID> route;
	uint departure_time;
	Speed speed;
};

class Schedule {
public:
	Schedule()
		: name("")
		, trains()
	{ }

	Schedule(
		const std::string& name,
		std::vector<Train>&& trains
	)
		: name(name)
		, trains(std::move(trains))
	{ }

	Schedule(const Schedule&) = default;
	Schedule(Schedule&&) = default;
	Schedule& operator=(const Schedule&) = default;
	Schedule& operator=(Schedule&&) = default;

	// getters
	const std::string& getName() const { return name; }
	std::vector<Train>& getTrains() { return trains; }
	const std::vector<Train>& getTrains() const { return trains; }

	Train& getTrain(TrainID id) { return getTrains()[id.getValue()]; }
	const Train& getTrain(TrainID id) const { return getTrains()[id.getValue()]; }

	template<typename MAPPED_TYPE, typename... ARGS>
	auto makeTrainMap(ARGS&&... args) {
		return std::vector<MAPPED_TYPE>(trains.size(), std::forward<ARGS>(args)...);
	}

private:
	std::string name;
	std::vector<Train> trains;
};

template<typename MAPPED_TYPE>
using TrainMap = decltype(Schedule().makeTrainMap<MAPPED_TYPE>());

/**
 * Main entry point into the scheduler
 *
 * Call this function to do scheduling
 */
Schedule schedule(
	const TrackNetwork& network,
	const PassengerList& passengers
);

} // end namespace algo



#endif /* SCHEDULER_H */