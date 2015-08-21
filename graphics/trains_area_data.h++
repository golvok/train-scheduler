
#ifndef UTIL__TRAINS_AREA_DATA_H
#define UTIL__TRAINS_AREA_DATA_H

#include <util/passenger.h++>
#include <util/track_network.h++>

#include <mutex>
#include <vector>

namespace graphics {

class TrainsArea;

/**
 * A class which acts as an interface for controlling the Trains Area
 * of the graphics, and for passing data to it.
 */
class TrainsAreaData {
public:
	TrainsAreaData();
	TrainsAreaData(const TrainsAreaData&) = delete;
	TrainsAreaData& operator=(const TrainsAreaData&) = delete;
	~TrainsAreaData();

	/**
	 * Call to clear all data, effectively clearing the TrainsArea
	 */
	void clear();

	/**
	 * Call to revert to newly constructed state:
	 * no TrainsArea associated, and no data.
	 */
	void reset();

	/**
	 * Cause the passed TrackNetwork to be displayed, nothing else.
	 */
	void displayTrackNetwork(TrackNetwork& new_tn);

	/**
	 * Cause the passed TrackNetwork and Passengers to be displayde,
	 * nothing else.
	 */
	void displayTNAndPassengers(
		TrackNetwork& new_tn,
		std::vector<Passenger>& new_passgrs
	);

	/**
	 * Display and Animate the results passed in
	 */
	void presentResults(
		TrackNetwork& new_tn,
		std::vector<Passenger>& new_passgrs
		// add more params later
	);
private:
	friend class TrainsArea;

	struct Data {
		TrackNetwork* tn;
		std::vector<Passenger>* passengers;
	};
	struct Results {
		// TRAINS trains;
	};
	struct Cache {
		class Impl;
		Impl* impl;
	};

	void setTrainsArea(TrainsArea* ta);
	bool hasTrainsArea();
	void clearCache();
	
	std::unique_lock<std::recursive_mutex> getScopedDataLock();

	bool hasTN();
	bool hasPassengers();
	bool hasTrains();

	TrackNetwork& getTN();
	std::vector<Passenger>& getPassengers();
	// TRAINS& getTrains();

	TrainsArea* trains_area;
	Data data;
	Results results;
	Cache cache;

	std::recursive_mutex data_mutex;
};

} // end namespace graphics

#endif /* UTIL__TRAINS_AREA_DATA_H */
