
#include "train_route.h++"

#include <util/logging.h++>

namespace algo {

namespace {

template<typename CONTAINER>
auto findFirstStartOffsetAfterOrAt(const TrackNetwork::Time& time_in_day, const CONTAINER& start_offsets) {
	return find_if(
		start_offsets.begin(), start_offsets.end(),
		[&](const auto& start_offset) {
			return start_offset >= time_in_day;
		}
	);
}

} // end anonymous namespace

const TrainRoute& Train::getRoute() const { return *train_route; }
RouteID Train::getRouteID() const { return train_route->getID(); }

TrainRoute::TrainRoute(
	const RouteID route_id,
	std::vector<TrackNetwork::ID>&& route_,
	std::vector<TrackNetwork::Time>&& start_offsets,
	TrackNetwork::Time repeat_time,
	const TrackNetwork& tn
)
	: route_id(route_id)
	, route(std::move(route_))
	, start_offsets(std::move(start_offsets))
	, repeat_time(repeat_time)
	, speed(0.5)
	, average_route_length_in_time(
		tn.sumTimeTakenWithCustomSpeed (
			::boost::make_iterator_range(route.begin(), route.end()),
			[&](const auto& edge) {
				(void)edge;
				return this->getSpeed();
			}
		)
	)
{ }

TrackNetwork::Time Train::getExpectedTravelTime(
	std::pair<TrackNetwork::ID, TrackNetwork::ID> first_and_last,
	const TrackNetwork& tn
) const {
	return train_route->getExpectedTravelTime(getDepartureTime(), first_and_last, tn);
}

TrackNetwork::Time Train::getExpectedTravelTime(
	::boost::iterator_range<RouteType::const_iterator> range,
	const TrackNetwork& tn
) const {
	return train_route->getExpectedTravelTime(getDepartureTime(), range, tn);
}

TrackNetwork::Time Train::getExpectedArrivalTime(
	TrackNetwork::ID to_here,
	const TrackNetwork& tn
) const {
	return getDepartureTime() + getExpectedTravelTime(
		std::make_pair(getRoute().getPath().front(), to_here),
		tn
	);
}

TrackNetwork::Time TrainRoute::getExpectedTravelTime(
	TrackNetwork::Time start_time,
	std::pair<TrackNetwork::ID, TrackNetwork::ID> first_and_last,
	const TrackNetwork& tn
) const {
	auto first_iter = std::find(route.begin(), route.end(), first_and_last.first);
	auto second_iter = std::find(first_iter, route.end(), first_and_last.second);
	if (first_iter == route.end()) { ::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
		str << "first vid not in route\n";
	});}
	if (second_iter == route.end()) { ::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
		str << "second vid not in route\n";
	});}
	return getExpectedTravelTime(
		start_time,
		{first_iter, second_iter + 1},
		tn
	);
}

TrackNetwork::Time TrainRoute::getExpectedTravelTime(
	TrackNetwork::Time start_time,
	::boost::iterator_range<RouteType::const_iterator> range,
	const TrackNetwork& tn
) const {
	(void)start_time; // TODO have variation?
	return tn.sumTimeTakenWithCustomSpeed(
		range,
		[&](const auto& edge) {
			(void)edge;
			return this->getSpeed();
		}
	);
}

std::pair<size_t,size_t> TrainRoute::getTrainsAtVertexInInterval_impl(
	TrackNetwork::ID vid,
	TrackNetwork::TimeInterval interval,
	const TrackNetwork& tn
) const {
	if (interval.first > interval.second) {
		::util::print_and_throw<std::invalid_argument>([&](auto&& err) {
			err << "interval is backward! : " << interval.first << " -/-> " << interval.second << '\n';
		});
	}

	const TrackNetwork::TimeInterval time_in_day(
		interval.first % repeat_time, interval.second % repeat_time
	);
	const TrackNetwork::TimeInterval day_number(
		interval.first / repeat_time, interval.second / repeat_time
	);

	const auto vid_in_route = std::find(getPath().begin(), getPath().end(), vid);
	if (vid_in_route == getPath().end()) {
		::util::print_and_throw<std::invalid_argument>([&](auto& msg) {
			msg << vid << " isn't in route";
		});
	}

	const auto time_in_route_to_vid = getExpectedTravelTime(
		*start_offsets.begin(),
		{getPath().begin(), vid_in_route},
		tn
	);

	const auto found_train_iters = std::make_pair(
		findFirstStartOffsetAfterOrAt(time_in_day.first - time_in_route_to_vid, start_offsets),
		findFirstStartOffsetAfterOrAt(time_in_day.second - time_in_route_to_vid, start_offsets)
	);

	return {
		day_number.first  * start_offsets.size() + std::distance(start_offsets.begin(), found_train_iters.first ),
		day_number.second * start_offsets.size() + std::distance(start_offsets.begin(), found_train_iters.second)
	};

}

Train TrainRoute::makeTrainFromIndex(size_t index) const {
	return Train(
		*this,
		repeat_time*(index / start_offsets.size())
			+ start_offsets[index % start_offsets.size()]
	);
}


} // end namespace algo
