#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <util/passenger.h++>
#include <util/track_network.h++>

namespace algo {

class Train {
public:
	using Speed = float;

	Train(
		const uint train_id,
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
	uint getId() const { return train_id; }
	auto& getRoute() { return route; }
	const auto& getRoute() const { return route; }
	TrackNetwork::ID getEntryId() const { return route.front(); }
	TrackNetwork::ID getExitId() const { return route.back(); }
	uint getDepartureTime() const { return departure_time; }
	Speed getSpeed() const { return speed; }

private:
	uint train_id;
	std::vector<TrackNetwork::ID> route;
	uint departure_time;
	Speed speed;
};

class Schedule {
	std::string name;
	std::vector<Train> trains;

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

		void addTrain();
};

/**
 * Main entry point into the scheduler
 *
 * Call this function to do scheduling
 */
Schedule schedule(
	const TrackNetwork& network,
	const std::vector<Passenger>& passengers
);

} // end namespace algo



#endif /* SCHEDULER_H */