
#ifndef ALGO__TRAIN_ROUTE_HPP
#define ALGO__TRAIN_ROUTE_HPP

#include <util/generator.h++>
#include <util/track_network.h++>

#include <boost/range/iterator_range.hpp>
#include <cstdint>

namespace algo {

struct RouteIdTagType {
	const static uint DEFAULT_VALUE = -1;
};
using RouteId = ::util::ID<uint,RouteIdTagType>;

class TrainRoute;
using RouteType = std::vector<TrackNetwork::ID>;

class Train {
public:
	friend class TrainRoute;
	using Speed = float;

	Train(
		const TrainRoute& train_route,
		TrackNetwork::Time departure_time
	)
		: train_route(&train_route)
		, departure_time(departure_time)
	{ }

	Train(const Train&) = default;
	Train(Train&&) = default;
	Train& operator=(const Train&) = default;
	Train& operator=(Train&&) = default;

	TrackNetwork::Time getExpectedTravelTime(
		std::pair<TrackNetwork::ID, TrackNetwork::ID> first_and_last,
		const TrackNetwork& tn
	) const;

	TrackNetwork::Time getExpectedTravelTime(
		::boost::iterator_range<RouteType::const_iterator> range,
		const TrackNetwork& tn
	) const;

	TrackNetwork::Time getExpectedArrivalTime(
		TrackNetwork::ID to_here,
		const TrackNetwork& tn
	) const;

	const TrainRoute& getRoute() const;
	TrackNetwork::Time getDepartureTime() const { return departure_time; }
	Speed getSpeed() const;
	RouteId getRouteID() const;

private:
	const TrainRoute* train_route;
	TrackNetwork::Time departure_time;
};

class TrainRoute {
public:

	TrainRoute(
		const RouteId route_id,
		std::vector<TrackNetwork::ID>&& route_,
		std::vector<TrackNetwork::Time>&& start_offsets,
		TrackNetwork::Time repeat_time,
		const TrackNetwork& tn
	);

	TrainRoute(const TrainRoute&) = delete;
	TrainRoute(TrainRoute&&) = default;
	TrainRoute& operator=(const TrainRoute&) = delete;
	TrainRoute& operator=(TrainRoute&&) = default;

	auto getTrainsAtVertexInInterval(
		TrackNetwork::ID vid,
		TrackNetwork::TimeInterval interval,
		const TrackNetwork& tn
	) const {
		auto data = getTrainsAtVertexInInterval_impl(vid, interval, tn);
		return util::xrange<size_t>(data.first, data.second, [&](const auto& index) {
			return this->makeTrainFromIndex(index);
		});
	}

	TrackNetwork::Time getExpectedTravelTime(
		TrackNetwork::Time start_time,
		std::pair<TrackNetwork::ID, TrackNetwork::ID> first_and_last,
		const TrackNetwork& tn
	) const;

	TrackNetwork::Time getExpectedTravelTime(
		TrackNetwork::Time start_time,
		::boost::iterator_range<RouteType::const_iterator> range,
		const TrackNetwork& tn
	) const;

	RouteId getID() const { return route_id; }
	const auto& getPath() const { return route; }
	Train::Speed getSpeed() const { return speed; }

private:
	std::pair<size_t,size_t> getTrainsAtVertexInInterval_impl(
		TrackNetwork::ID vid,
		TrackNetwork::TimeInterval interval,
		const TrackNetwork& tn
	) const;

	Train makeTrainFromIndex(size_t index) const;

	const RouteId route_id;
	const RouteType route;
	const std::vector<TrackNetwork::Time> start_offsets;
	const TrackNetwork::Time repeat_time;
	const Train::Speed speed;
	const TrackNetwork::Time average_route_length_in_time;
};

} // ende namespace algo

#endif /* ALGO__TRAIN_ROUTE_HPP */
