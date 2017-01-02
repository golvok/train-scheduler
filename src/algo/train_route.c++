
#include "train_route.h++"

#include <util/logging.h++>
#include <util/routing_utils.h++>
#include <util/utils.h++>

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
	std::vector<TrackNetwork::NodeID>&& route_,
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

TrainRoute::TrainRoute(
	const RouteID route_id,
	const std::vector<TrackNetwork::NodeID>& route,
	std::vector<TrackNetwork::Time>&& start_offsets,
	TrackNetwork::Time repeat_time,
	const TrackNetwork& tn
)
	: TrainRoute(
		route_id,
		std::vector<TrackNetwork::NodeID>(route),
		std::move(start_offsets),
		repeat_time,
		tn
	)
{ }


TrackNetwork::Time Train::getExpectedTravelTime(
	::boost::iterator_range<RouteType::const_iterator> range,
	const TrackNetwork& tn
) const {
	return train_route->getExpectedTravelTime(getDepartureTime(), range, tn);
}

TrackNetwork::Time Train::getExpectedArrivalTime(
	RouteType::const_iterator to_here,
	const TrackNetwork& tn
) const {
	return getDepartureTime() + train_route->getExpectedTravelTime(getDepartureTime(), to_here, tn);
}

TrackNetwork::Time Train::getExpectedArrivalTime(
	TrackNetwork::NodeID to_here,
	size_t matches_to_skip,
	const TrackNetwork& tn
) const {
	const auto there_iter = ::util::skip_find(getRoute().getPath(), matches_to_skip, to_here);

	if (there_iter == end(getRoute().getPath())) {
		::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
			str << "stop VID is not in route";
		});
	}

	return getDepartureTime() + getExpectedTravelTime(
		{
			begin(getRoute().getPath()),
			next(there_iter)
		},
		tn
	);
}

RouteType::const_iterator Train::getNextPathIterAfterTime(
	TrackNetwork::Time t,
	const TrackNetwork& tn
) const {
	using std::end;
	for (const auto& it : util::iterate_with_iterators(getRoute().getPath())) {
		const auto arrival_time = getExpectedArrivalTime(it, tn);
		if (arrival_time > t) {
			return it;
		}
	}
	return end(getRoute().getPath());
}

void Train::print(std::ostream& os) const {
	getTrainID().print(os);
}

TrackNetwork::Time TrainRoute::getExpectedTravelTime(
	TrackNetwork::Time start_time,
	std::pair<TrackNetwork::NodeID, TrackNetwork::NodeID> first_and_last,
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

TrackNetwork::Time TrainRoute::getExpectedTravelTime(
	TrackNetwork::Time start_time,
	RouteType::const_iterator to_here,
	const TrackNetwork& tn
) const {
	using std::begin; using std::next;
	return getExpectedTravelTime(start_time, {begin(getPath()), to_here}, tn);
}

std::pair<TrainIndex,TrainIndex> TrainRoute::getTrainsAtVertexInInterval_impl(
	TrackNetwork::NodeID vid,
	TrackNetwork::TimeInterval interval,
	size_t matches_to_skip,
	const TrackNetwork& tn
) const {
	if (interval.first > interval.second) {
		::util::print_and_throw<std::invalid_argument>([&](auto&& err) {
			err << "interval is backward! : " << interval.first << " -/-> " << interval.second << '\n';
		});
	}

	const TrackNetwork::TimeInterval time_in_day(
		std::fmod(interval.first,repeat_time), std::fmod(interval.second,repeat_time)
	);
	const TrackNetwork::TimeInterval day_number(
		interval.first / repeat_time, interval.second / repeat_time
	);

	const auto vid_in_route = ::util::skip_find(getPath(), matches_to_skip, vid);

	if (vid_in_route == getPath().end()) {
		return {-1, -1}; // return an empty range
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

Train TrainRoute::makeTrainFromIndex(TrainIndex index) const {
	return Train(
		*this,
		repeat_time*(index / start_offsets.size())
			+ start_offsets[index % start_offsets.size()],
		index
	);
}

void TrainRoute::print(std::ostream& os, const TrackNetwork& tn) const {
	os << "{ Train " << route_id << " : Path=";
	::util::print_route(route, tn, os);
	os << ", Start Offsets=";
	::util::print_container(start_offsets, os, " -> ", "{ ", " }", [&](auto& os, auto& elem) {
		os << tn.getVertexName(elem);
	});
	os << ", Speed=" << speed << ", Repeat Time=" << repeat_time << " }";
}

} // end namespace algo
