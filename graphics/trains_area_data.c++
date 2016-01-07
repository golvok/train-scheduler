#include "trains_area_data.h++"


#include <algo/passenger_routing.h++>
#include <graphics/trains_area.h++>
#include <util/utils.h++>

#include <cassert>

namespace graphics {

class TrainsAreaData::Cache::Impl {
public:
	void clearAll() { }
	void clearTNRelated() { }
	void clearPassengerRelated() { }
};

TrainsAreaData::TrainsAreaData()
	: trains_area(nullptr)
	, data{}
	, cache{}
	, data_mutex()
{
	clear();
}

TrainsAreaData::~TrainsAreaData() {
	auto sdl = getScopedDataLock();
	if (hasTrainsArea()) {
		trains_area->stopAnimating();
	}
}

void TrainsAreaData::clear() {
	auto sdl = getScopedDataLock();
	cache.impl->clearAll();

	data.tn.reset();
	data.passengers.reset();

	data.schedule.reset();

	if (hasTrainsArea()) {
		trains_area->stopAnimating();
	}
}

void TrainsAreaData::displayTrackNetwork(
	std::weak_ptr<TrackNetwork> new_tn
) {
	auto sdl = getScopedDataLock();
	clear();

	data.tn = new_tn;

	if (hasTrainsArea()) {
		trains_area->stopAnimating();
		trains_area->forceRedraw();
	}
}

void TrainsAreaData::displayTNAndPassengers(
	std::weak_ptr<TrackNetwork> new_tn,
	std::weak_ptr<PassengerList> new_passgrs
) {
	auto sdl = getScopedDataLock();
	clear();

	displayTrackNetwork(new_tn);

	data.passengers = new_passgrs;

	if (hasTrainsArea() && getPassengers()) {
		trains_area->resetAnimationTime();
		trains_area->beginAnimating();
	}
}

void TrainsAreaData::displayTNAndWantedCapacities(
	// std::weak_ptr<TrackNetwork> new_tn,
	std::weak_ptr<Data::WantedCapacityMap> new_wanted_edge_capacities
) {
	auto sdl = getScopedDataLock();
	// clear();

	// displayTrackNetwork(new_tn);
	data.wanted_edge_capacities = new_wanted_edge_capacities;
}

void TrainsAreaData::presentResults(
	std::weak_ptr<TrackNetwork> new_tn,
	std::weak_ptr<PassengerList> new_passgrs,
	std::weak_ptr<algo::Schedule> new_schedule
) {
	auto sdl = getScopedDataLock();
	clear();

	displayTNAndPassengers(new_tn, new_passgrs);
	data.schedule = new_schedule;
}

void TrainsAreaData::presentResultsWithRoutedPassengers(
	std::weak_ptr<TrackNetwork> new_tn,
	std::weak_ptr<PassengerList> new_passgrs,
	std::weak_ptr<algo::Schedule> new_schedule,
	std::weak_ptr<::algo::PassengerRoutes> new_passenger_routes
) {
	auto sdl = getScopedDataLock();
	clear();

	displayTNAndPassengers(new_tn, new_passgrs);
	data.schedule = new_schedule;
	data.passenger_routes = new_passenger_routes;
}

void TrainsAreaData::setTrainsArea(TrainsArea* ta) {
	assert(ta != nullptr || !hasTrainsArea());
	trains_area = ta;
}

bool TrainsAreaData::hasTrainsArea() {
	auto sdl = getScopedDataLock();
	return trains_area != nullptr;
}

std::unique_lock<std::recursive_mutex> TrainsAreaData::getScopedDataLock() {
	return std::unique_lock<std::recursive_mutex> (data_mutex);
}

std::shared_ptr<TrackNetwork> TrainsAreaData::getTN() {
	auto sdl = getScopedDataLock();
	return data.tn.lock();
}

std::shared_ptr<PassengerList> TrainsAreaData::getPassengers() {
	auto sdl = getScopedDataLock();
	return data.passengers.lock();
}

std::shared_ptr<algo::Schedule> TrainsAreaData::getSchedule() {
	auto sdl = getScopedDataLock();
	return data.schedule.lock();
}

std::shared_ptr<TrainsAreaData::Data::WantedCapacityMap> TrainsAreaData::getWantedEdgeCapacities() {
	auto sdl = getScopedDataLock();
	return data.wanted_edge_capacities.lock();
}

std::shared_ptr<::algo::PassengerRoutes> TrainsAreaData::getPassengerRoutes() {
	auto sdl = getScopedDataLock();
	return data.passenger_routes.lock();
}

} // end namespace graphics
