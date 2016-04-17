
#ifndef ALGO__PASSENGER_ROUTING_HPP
#define ALGO__PASSENGER_ROUTING_HPP

#include <util/handles.h++>
#include <util/location_id.h++>
#include <util/passenger.h++>

#include <unordered_map>
#include <vector>

namespace algo {

class Schedule;

class PassengerRoutes {
public:
	class RouteElement {
		LocationID location;
		TrackNetwork::Time time;
	public:
		RouteElement(LocationID location, TrackNetwork::Time time)
			: location(location), time(time)
		{ }

		RouteElement() : location(), time(0) { }
		RouteElement(const RouteElement&) = default;
		RouteElement(RouteElement&&) = default;
		RouteElement& operator=(const RouteElement&) = default;
		RouteElement& operator=(RouteElement&&) = default;

		auto getLocation() const { return location; }
		auto getTime() const { return time; }
	};
	using InternalRouteType = std::vector<RouteElement>;
	using RouteType = InternalRouteType; // for now...
private:
	std::unordered_map<PassengerId,InternalRouteType> routes;
public:

	PassengerRoutes() : routes() { }

	template<typename ROUTE>
	void addRoute(const Passenger& p, ROUTE&& route) {
		if (route.empty()) {
			throw std::invalid_argument(
				std::string(__PRETTY_FUNCTION__) + ": attempt to insert empty route for passenger " + p.getName()
			);
		}
		auto emplace_results = routes.emplace(p.getID(), std::forward<ROUTE>(route));
		if (emplace_results.second == false) {
			emplace_results.first->second = std::forward<ROUTE>(route);
		}
	}

	const RouteType& getRoute(const Passenger& p) const {
		auto find_results = routes.find(p.getID());
		if (find_results == routes.end()) {
			throw std::invalid_argument(
				std::string(__PRETTY_FUNCTION__) + ": do not have route for passenger " + p.getName()
			);
		}
		return find_results->second;
	}
};

PassengerRoutes route_passengers(
	const TrackNetwork& tn,
	const Schedule& sch,
	const PassengerList& passgrs
);

struct RouteTroughScheduleCache;
struct RouteTroughScheduleCacheHandle : public ::util::unique_handle<RouteTroughScheduleCache> {
	using unique_handle::unique_handle;
	using unique_handle::operator=;
	RouteTroughScheduleCacheHandle(RouteTroughScheduleCacheHandle&&);
	RouteTroughScheduleCacheHandle& operator=(RouteTroughScheduleCacheHandle&&);
	RouteTroughScheduleCacheHandle();
	~RouteTroughScheduleCacheHandle();
};

std::pair<
	PassengerRoutes::RouteType,
	RouteTroughScheduleCacheHandle
> route_through_schedule(
	const TrackNetwork& tn,
	const Schedule& sch,
	const TrackNetwork::NodeID start_vertex,
	const TrackNetwork::NodeID goal_vertex,
	RouteTroughScheduleCacheHandle&& cache_handle = RouteTroughScheduleCacheHandle()
);

} // end namespace algo

#endif /* ALGO__PASSENGER_ROUTING_HPP */
