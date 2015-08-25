#include "trains_area_data.h++"

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
	, results{}
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
	std::weak_ptr<std::vector<Passenger>> new_passgrs
) {
	auto sdl = getScopedDataLock();
	clear();

	displayTrackNetwork(new_tn);

	data.passengers = new_passgrs;

	if (hasTrainsArea() && hasPassengers()) {
		trains_area->resetAnimationTime();
		trains_area->beginAnimating();
	}
}

void TrainsAreaData::presentResults(
	std::weak_ptr<TrackNetwork> new_tn,
	std::weak_ptr<std::vector<Passenger>> new_passgrs
) {
	auto sdl = getScopedDataLock();
	clear();

	displayTNAndPassengers(new_tn, new_passgrs);
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

bool TrainsAreaData::hasTN()         {
	auto sdl = getScopedDataLock();
	return data.tn.expired() == false;
}

bool TrainsAreaData::hasPassengers() {
	auto sdl = getScopedDataLock();
	return data.passengers.expired() == false;
}

bool TrainsAreaData::hasTrains()     {
	auto sdl = getScopedDataLock();
	return false;
}

std::shared_ptr<TrackNetwork> TrainsAreaData::getTN() {
	auto sdl = getScopedDataLock();
	assert(hasTN());
	return data.tn.lock();
}

std::shared_ptr<std::vector<Passenger>> TrainsAreaData::getPassengers() {
	auto sdl = getScopedDataLock();
	assert(hasPassengers());
	return data.passengers.lock();
}

// std::shared_ptr<TRAINS> getTrains() {
// 	auto sdl = getScopedDataLock();
// 	assert(hasTrains());
// 	return results.trains.lock();
// }

} // end namespace graphics
