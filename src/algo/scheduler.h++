#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <algo/train_route.h++>
#include <util/passenger.h++>
#include <util/track_network.h++>
#include <util/utils.h++>

namespace algo {

class Schedule {
public:
	Schedule()
		: name("")
		, train_routes()
	{ }

	Schedule(
		const std::string& name,
		std::vector<TrainRoute>&& train_routes
	)
		: name(name)
		, train_routes(std::move(train_routes))
	{ }

	Schedule(const Schedule&) = delete;
	Schedule(Schedule&&) = default;
	Schedule& operator=(const Schedule&) = delete;
	Schedule& operator=(Schedule&&) = default;

	// getters
	const std::string& getName() const { return name; }

	TrainRoute& getTrainRoute(RouteID id) { return train_routes.at(id.getValue()); }
	const TrainRoute& getTrainRoute(RouteID id) const { return train_routes.at(id.getValue()); }
	const auto& getTrainRoutes() const { return train_routes; }

	template<typename MAPPED_TYPE, typename... ARGS>
	auto makeRouteMap(ARGS&&... args) const {
		return std::vector<MAPPED_TYPE>(train_routes.size(), std::forward<ARGS>(args)...);
	}

	template<typename MAPPED_TYPE, typename... ARGS>
	auto makeTrainMap(ARGS&&... args) const {
		return ::util::with_my_hash_t<std::unordered_map,TrainID,MAPPED_TYPE>(std::forward<ARGS>(args)...);
	}
private:
	std::string name;
	std::vector<TrainRoute> train_routes;
};

template<typename MAPPED_TYPE>
using RouteMap = decltype(Schedule().makeRouteMap<MAPPED_TYPE>());

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