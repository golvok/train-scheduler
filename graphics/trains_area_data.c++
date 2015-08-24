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
	, data{nullptr,nullptr}
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
	data.tn = nullptr;
	data.passengers = nullptr;
	cache.impl->clearAll();

	if (hasTrainsArea()) {
		trains_area->stopAnimating();
	}
}

void TrainsAreaData::displayTrackNetwork(TrackNetwork& new_tn) {
	auto sdl = getScopedDataLock();

	if (data.tn != &new_tn) {
		cache.impl->clearTNRelated();
		data.tn = &new_tn;
	}

	if (hasTrainsArea()) {
		trains_area->stopAnimating();
		trains_area->forceRedraw();
	}
}

void TrainsAreaData::displayTNAndPassengers(TrackNetwork& new_tn, std::vector<Passenger>& new_passgrs) {
	auto sdl = getScopedDataLock();

	if (data.passengers != &new_passgrs) {
		cache.impl->clearPassengerRelated();
		data.passengers = &new_passgrs;
	}

	displayTrackNetwork(new_tn);

	if (hasTrainsArea() && hasPassengers()) {
		trains_area->resetAnimationTime();
		trains_area->beginAnimating();
	}
}

void TrainsAreaData::presentResults(
	TrackNetwork& new_tn,
	std::vector<Passenger>& new_passgrs
) {
	auto sdl = getScopedDataLock();
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
	return data.tn != nullptr;
}

bool TrainsAreaData::hasPassengers() {
	auto sdl = getScopedDataLock();
	return data.passengers != nullptr;
}

bool TrainsAreaData::hasTrains()     {
	auto sdl = getScopedDataLock();
	return false;
}

TrackNetwork& TrainsAreaData::getTN() {
	auto sdl = getScopedDataLock();
	assert(hasTN());
	return *data.tn;
}

std::vector<Passenger>& TrainsAreaData::getPassengers() {
	auto sdl = getScopedDataLock();
	assert(hasPassengers());
	return *data.passengers;
}

// TRAINS& getTrains() { return results.trains; }


} // end namespace graphics
