#include "trains_area_data.h++"


#include <algo/passenger_routing.h++>
#include <graphics/trains_area.h++>
#include <util/logging.h++>
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
	, alive_flag_and_mutex(std::make_shared<std::pair<bool,std::mutex>>())
	, sim_wait_cv()
	, sim_frame_drawn(false)
{
	alive_flag_and_mutex->first = true;
	clear();
}

TrainsAreaData::~TrainsAreaData() {
	// make it look like the frame has been drawn
	notifyOfFrameDrawn();
	// wait for it to exit
	std::unique_lock<std::mutex> alive_ul(alive_flag_and_mutex->second);
	// set alive = false, now that we have the lock
	alive_flag_and_mutex->first = false;

	// stop refreshing & clean up
	auto sdl = getScopedDataLock();
	if (hasTrainsArea()) {
		trains_area->stopAnimating();
	}
}

void TrainsAreaData::clear() {
	auto tn_ptr_copy = data.tn;
	auto passengers_ptr_copy = data.passengers;
	auto schedule_ptr_copy = data.schedule;
	auto simulator_ptr_copy = data.simulator;

	{
		auto sdl = getScopedDataLock();

		if (hasTrainsArea()) {
			trains_area->stopAnimating();
			notifyOfFrameDrawn();
		}

		cache.impl->clearAll();

		data.tn.reset();
		data.passengers.reset();
		data.schedule.reset();
		data.simulator.reset();
	}
}

void TrainsAreaData::displayTrackNetwork(
	std::weak_ptr<const TrackNetwork> new_tn
) {
	clear();
	auto sdl = getScopedDataLock();

	data.tn = new_tn;

	if (hasTrainsArea()) {
		trains_area->stopAnimating();
		trains_area->forceRedraw();
	}
}

void TrainsAreaData::displayTNAndPassengers(
	std::weak_ptr<const TrackNetwork> new_tn,
	std::weak_ptr<const PassengerList> new_passgrs
) {
	clear();
	auto sdl = getScopedDataLock();

	displayTrackNetwork(new_tn);

	data.passengers = new_passgrs;

	if (hasTrainsArea() && getPassengers()) {
		trains_area->resetAnimationTime();
		trains_area->beginAnimating();
	}
}

void TrainsAreaData::displayTNAndWantedCapacities(
	// std::weak_ptr<const TrackNetwork> new_tn,
	std::weak_ptr<const Data::WantedCapacityMap> new_wanted_edge_capacities
) {
	// clear();
	auto sdl = getScopedDataLock();

	// displayTrackNetwork(new_tn);
	data.wanted_edge_capacities = new_wanted_edge_capacities;
}

void TrainsAreaData::presentResults(
	std::weak_ptr<const TrackNetwork> new_tn,
	std::weak_ptr<const PassengerList> new_passgrs,
	std::weak_ptr<const algo::Schedule> new_schedule
) {
	clear();
	auto sdl = getScopedDataLock();

	displayTNAndPassengers(new_tn, new_passgrs);
	data.schedule = new_schedule;
}

void TrainsAreaData::displaySimulator(
	::sim::SimulatorHandle new_simulator
) {
	clear();
	auto sdl = getScopedDataLock();

	displayTrackNetwork(new_simulator.getTrackNetworkUsed());
	data.schedule = new_simulator.getScheduleUsed();
	data.simulator = new_simulator;

	struct SimulateCheck {
		bool operator()(TrainsAreaData& tad) const {
			return tad.hasTrainsArea() && tad.getSchedule() && tad.getSimulatorHandle();
		}
	};

	struct WaitForDrawCheck {
		WaitForDrawCheck(
			TrainsAreaData& tad
		)
			: local_alive_flag_and_mutex(tad.alive_flag_and_mutex)
			, tad(tad)
		{ }

		// want a local copy of the alive flag & mutex so that the sp lifetime is
		// tied to this object. (note: binding to a std::function is by copy)
		std::shared_ptr<std::pair<bool, std::mutex>> local_alive_flag_and_mutex;

		TrainsAreaData& tad;

		bool operator()() {

			// first, lock and check the alive bool
			std::unique_lock<std::mutex> alive_ul(local_alive_flag_and_mutex->second);
			if (local_alive_flag_and_mutex->first == false) {
				return false;
			}

			// if this TrainsAreaData is still around, check if we should
			// wait for the frame to be drawn
			std::unique_lock<std::recursive_mutex> data_ul(tad.data_mutex);
			tad.sim_frame_drawn = false; // mark frame as not drawn
			tad.sim_wait_cv.wait(data_ul, [&]() {
				return !(SimulateCheck()(tad) && !tad.sim_frame_drawn); // it's while (!pred) inside .wait()
			});


			// keep this observer active iff this is still watching the simulation
			return SimulateCheck()(tad);
		}
	};

	if (SimulateCheck()(*this)) {
		data.simulator.registerObserver(
			WaitForDrawCheck(*this),
			1.1
		);
		trains_area->resetAnimationTime();
		trains_area->beginAnimating();
	}


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

std::shared_ptr<const TrackNetwork> TrainsAreaData::getTN() {
	auto sdl = getScopedDataLock();
	return data.tn.lock();
}

std::shared_ptr<const PassengerList> TrainsAreaData::getPassengers() {
	auto sdl = getScopedDataLock();
	return data.passengers.lock();
}

std::shared_ptr<const algo::Schedule> TrainsAreaData::getSchedule() {
	auto sdl = getScopedDataLock();
	return data.schedule.lock();
}

std::shared_ptr<const TrainsAreaData::Data::WantedCapacityMap> TrainsAreaData::getWantedEdgeCapacities() {
	auto sdl = getScopedDataLock();
	return data.wanted_edge_capacities.lock();
}

::sim::SimulatorHandle TrainsAreaData::getSimulatorHandle() {
	auto sdl = getScopedDataLock();
	return data.simulator;
}

void TrainsAreaData::notifyOfFrameDrawn() {
	auto sdl = getScopedDataLock();
	sim_frame_drawn = true;

	// wake up any observers (if they are waiting)
	sim_wait_cv.notify_all();
}

} // end namespace graphics
