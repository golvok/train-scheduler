
#ifndef UTIL__TRAINS_AREA_DATA_H
#define UTIL__TRAINS_AREA_DATA_H

#include <algo/scheduler.h++>
#include <sim/simulator.h++>
#include <util/passenger.h++>
#include <util/track_network.h++>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

namespace graphics {

class TrainsArea;

/**
 * A class which acts as an interface for controlling the Trains Area
 * of the graphics, and for passing data to it.
 */
class TrainsAreaData {
private:
	friend class TrainsArea;

	struct Data {
		using WantedCapacityMap = std::vector<float>;

		// stored using weak_ptr so that lifetime corresponds to external lifetime
		std::weak_ptr<const TrackNetwork> tn;
		std::weak_ptr<const PassengerList> passengers;
		std::weak_ptr<const WantedCapacityMap> wanted_edge_capacities;
		std::weak_ptr<const algo::Schedule> schedule;
		::sim::SimulatorHandle simulator;
	};
	struct Cache {
		// PIMPL because Impl may contain windowing/drawing dependencies
		class Impl;
		Impl* impl;
	};

public:
	TrainsAreaData();
	TrainsAreaData(const TrainsAreaData&) = delete;
	TrainsAreaData& operator=(const TrainsAreaData&) = delete;
	virtual ~TrainsAreaData();

	/**
	 * Call to clear all data, effectively clearing the TrainsArea
	 */
	void clear();

	/**
	 * Cause the passed TrackNetwork to be displayed, nothing else.
	 */
	void displayTrackNetwork(
		std::weak_ptr<const TrackNetwork> new_tn);

	/**
	 * Cause the passed TrackNetwork and Passengers to be displayed,
	 * nothing else.
	 */
	void displayTNAndPassengers(
		std::weak_ptr<const TrackNetwork> new_tn,
		std::weak_ptr<const PassengerList> new_passgrs
	);

	void displayTNAndWantedCapacities(
		// std::weak_ptr<const TrackNetwork> new_tn,
		std::weak_ptr<const Data::WantedCapacityMap> new_wanted_edge_capacities
	);

	/**
	 * Display and Animate the results passed in
	 */
	void presentResults(
		std::weak_ptr<const TrackNetwork> new_tn,
		std::weak_ptr<const PassengerList> new_passgrs,
		std::weak_ptr<const algo::Schedule> new_schedule
	);

	/**
	 * Display and Animate the results passed in
	 */
	void displaySimulator(
		::sim::SimulatorHandle new_simulator
	);

private:
	void setTrainsArea(TrainsArea* ta);
	bool hasTrainsArea();

	/**
	 * This function guarantees that no data object will be replaced
	 * while it's return value is in scope. However, it does not guarantee that
	 * the objects will not be freed by the shared_ptr count going to zero. This
	 * is considered okay, as the case of no data is usually easily handled as do nothing.
	 */
	std::unique_lock<std::recursive_mutex> getScopedDataLock();

	// The various data accessor functions.

	// A shared_ptr is returned so that the data will not be freed while it is in use. However,
	// these functions can return values that are not consistent with each other, as another
	// thread could replace one datum with a new one between calls to each getter. Therefore
	// when making calls to more than one of these functions, if you want values that are
	// guaranteed to be consistent with each other, or empty, use getScopedDataLock(). See also
	// documentation for that function about possibly freed data.
	//
	// Note: the returned value may not point to a valid object, and that should be checked
	//     before use!
	std::shared_ptr<const TrackNetwork> getTN();
	std::shared_ptr<const PassengerList> getPassengers();
	std::shared_ptr<const algo::Schedule> getSchedule();
	std::shared_ptr<const Data::WantedCapacityMap> getWantedEdgeCapacities();
	::sim::SimulatorHandle getSimulatorHandle();

	void notifyOfFrameDrawn();

	TrainsArea* trains_area;
	Data data;
	Cache cache;

	std::recursive_mutex data_mutex;

	std::shared_ptr<std::pair<bool, std::mutex>> alive_flag_and_mutex;

	std::condition_variable_any sim_wait_cv;
	bool sim_frame_drawn;
};

} // end namespace graphics

#endif /* UTIL__TRAINS_AREA_DATA_H */
