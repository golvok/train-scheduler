
#ifndef ALGO__TRAIN_ROUTE_HPP
#define ALGO__TRAIN_ROUTE_HPP

#include <util/generator.h++>
#include <util/track_network.h++>

#include <boost/range/iterator_range.hpp>
#include <cstdint>

class LocationID;

namespace algo {

struct RouteIDTagType {
	const static uint32_t DEFAULT_VALUE = -1;
};
using RouteID = ::util::ID<uint32_t,RouteIDTagType>;

class TrainRoute;
using RouteType = std::vector<TrackNetwork::ID>;
using TrainIndex = uint32_t;

struct TrainIDTagType {
	const static uint64_t DEFAULT_VALUE = -1;
};
class TrainID : public ::util::ID<uint64_t,TrainIDTagType> {
	friend class ::LocationID;
	TrainID(IDType val) : ID(val) { }
public:

	TrainID(RouteID rid, TrainIndex train_index)
		: ID(((uint64_t)rid.getValue() << (sizeof(TrainIndex)*CHAR_BIT)) | train_index) { }
	TrainID() : ID() { }

	RouteID getRouteID() const {
		return ::util::make_id<RouteID>(getValue() >> (sizeof(TrainIndex)*CHAR_BIT));
	}
	TrainIndex getTrainIndex() const {
		return static_cast<TrainIndex>(getValue() & static_cast<TrainIndex>(-1));
	}

	void print(std::ostream& os) const {
		if (getValue() == DEFAULT_VALUE) {
			os << "<DEFAULT_TRAIN>";
		} else {
			os << 'r' << getRouteID().getValue() << 't' << getTrainIndex();
		}
	}
};

class Train {
public:
	friend class TrainRoute;
	using Speed = float;

	Train(
		const TrainRoute& train_route,
		TrackNetwork::Time departure_time,
		TrainIndex index_number
	)
		: train_route(&train_route)
		, departure_time(departure_time)
		, index_number(index_number)
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
	RouteID getRouteID() const;
	TrainID getTrainID() const {
		return ::util::make_id<TrainID>(getRouteID(), index_number);
	}

	void print(std::ostream& os) const;
private:
	const TrainRoute* train_route;
	TrackNetwork::Time departure_time;
	TrainIndex index_number;
};

inline std::ostream& operator<<(std::ostream& os, const Train& t) {
	t.print(os);
	return os;
}

class TrainRoute {
public:

	TrainRoute(
		const RouteID route_id,
		std::vector<TrackNetwork::ID>&& route_,
		std::vector<TrackNetwork::Time>&& start_offsets,
		TrackNetwork::Time repeat_time,
		const TrackNetwork& tn
	);

	TrainRoute(const TrainRoute&) = delete;
	TrainRoute(TrainRoute&&) = default;
	TrainRoute& operator=(const TrainRoute&) = delete;
	TrainRoute& operator=(TrainRoute&&) = default;

	auto getTrainsAtVertexInInterval( // TODO rename to indicate generator return value
		TrackNetwork::ID vid,
		TrackNetwork::TimeInterval interval,
		const TrackNetwork& tn
	) const {
		auto data = getTrainsAtVertexInInterval_impl(vid, interval, tn);
		return ::util::make_generator<TrainIndex>(
			data.first,
			data.second,
			[&](const auto& index) {
				return index + 1;
			},
			[&](const auto& index) {
				return this->makeTrainFromIndex(index);
			}
		);
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

	RouteID getID() const { return route_id; }
	const auto& getPath() const { return route; }
	Train::Speed getSpeed() const { return speed; }

	auto getTrainsLeavingInInterval( // TODO rename to indicate generator return value
		TrackNetwork::TimeInterval interval,
		const TrackNetwork& tn
	) const {
		return getTrainsAtVertexInInterval(getPath().front(), interval, tn);
	}

	Train makeTrainFromIndex(TrainIndex index) const;

private:
	std::pair<TrainIndex,TrainIndex> getTrainsAtVertexInInterval_impl(
		TrackNetwork::ID vid,
		TrackNetwork::TimeInterval interval,
		const TrackNetwork& tn
	) const;

	const RouteID route_id;
	const RouteType route;
	const std::vector<TrackNetwork::Time> start_offsets;
	const TrackNetwork::Time repeat_time;
	const Train::Speed speed;
	const TrackNetwork::Time average_route_length_in_time;
};

} // ende namespace algo

namespace std {
	template<>
	struct hash<::algo::Train> {
		size_t operator()(const ::algo::Train& t) const {
			const auto& v = t.getTrainID().getValue();
			return std::hash<std::remove_cv<std::remove_reference<decltype(v)>::type>::type>()(v);
		}
	};
}

#endif /* ALGO__TRAIN_ROUTE_HPP */
