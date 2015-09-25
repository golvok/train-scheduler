
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

		std::weak_ptr<TrackNetwork> tn;
		std::weak_ptr<std::vector<Passenger>> passengers;
		std::weak_ptr<WantedCapacityMap> wanted_edge_capacities;
	};
	struct Results {
		std::weak_ptr<algo::Schedule> schedule;
	};
	struct Cache {
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
		std::weak_ptr<std::vector<Passenger>> new_passgrs
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
		std::weak_ptr<std::vector<Passenger>> new_passgrs,
		std::weak_ptr<algo::Schedule> new_schedule
	);

private:
	void setTrainsArea(TrainsArea* ta);
	bool hasTrainsArea();

	std::unique_lock<std::recursive_mutex> getScopedDataLock();

	std::shared_ptr<TrackNetwork> getTN();
	std::shared_ptr<std::vector<Passenger>> getPassengers();
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
