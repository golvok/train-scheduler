
#ifndef UTIL__TRAINS_AREA_DATA_H
#define UTIL__TRAINS_AREA_DATA_H

#include <util/passenger.h++>
#include <util/track_network.h++>
#include <algo/scheduler.h++>

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
		std::weak_ptr<TrackNetwork> tn;
		std::weak_ptr<PassengerList> passengers;
		std::weak_ptr<WantedCapacityMap> wanted_edge_capacities;
	};
	struct Results {
		// stored using weak_ptr so that lifetime corresponds to external lifetime
		std::weak_ptr<algo::Schedule> schedule;
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
		std::weak_ptr<TrackNetwork> new_tn);

	/**
	 * Cause the passed TrackNetwork and Passengers to be displayed,
	 * nothing else.
	 */
	void displayTNAndPassengers(
		std::weak_ptr<TrackNetwork> new_tn,
		std::weak_ptr<PassengerList> new_passgrs
	);

	void displayTNAndWantedCapacities(
		// std::weak_ptr<TrackNetwork> new_tn,
		std::weak_ptr<Data::WantedCapacityMap> new_wanted_edge_capacities
	);

	/**
	 * Display and Animate the results passed in
	 */
	void presentResults(
		std::weak_ptr<TrackNetwork> new_tn,
		std::weak_ptr<PassengerList> new_passgrs,
		std::weak_ptr<algo::Schedule> new_schedule
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
	std::shared_ptr<TrackNetwork> getTN();
	std::shared_ptr<PassengerList> getPassengers();
	std::shared_ptr<algo::Schedule> getSchedule();
	std::shared_ptr<Data::WantedCapacityMap> getWantedEdgeCapacities();

	TrainsArea* trains_area;
	Data data;
	Results results;
	Cache cache;

	std::recursive_mutex data_mutex;
};

} // end namespace graphics

#endif /* UTIL__TRAINS_AREA_DATA_H */
