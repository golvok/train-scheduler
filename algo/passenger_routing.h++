
#ifndef ALGO__PASSENGER_ROUTING_HPP
#define ALGO__PASSENGER_ROUTING_HPP

#include <util/passenger.h++>

class Schedule;

namespace algo {

int route_passengers(
	const TrackNetwork& tn,
	const Schedule& sch,
	const std::vector<Passenger>& passgrs
);

} // end namespace algo

#endif /* ALGO__PASSENGER_ROUTING_HPP */
