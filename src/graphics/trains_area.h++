
#ifndef GRAPHICS__TRAINS_AREA_H
#define GRAPHICS__TRAINS_AREA_H

#include "graphics.h++"
#include <graphics/windowing_includes.h++>
#include <util/thread_utils.h++>

#include <cstdint>
#include <mutex>

namespace graphics {

class TrainsArea : public Gtk::DrawingArea {
public:

	TrainsArea(TrainsAreaData& data);
	virtual ~TrainsArea() { }

protected:
	struct PassengerLocations {
		::algo::TrainMap<PassengerIDList> passengers_on_trains;
		StationMap<PassengerIDList> passengers_at_stations;

		PassengerLocations() : passengers_on_trains(), passengers_at_stations() { }
	};

	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cc) override;

	// The various drawing functions. Each does nothing if the data it needs isn't available
	// or if some state information isn't what it wants (eg. is/isn't animating)
	// Each function is only expected to have consistency within itself, so
	// a caller must also lock data & state info to have guaranteed consistency
	// between individual calls
	void centerOnTrackNework(const Cairo::RefPtr<Cairo::Context>& cc);
	void drawTrackNetwork(const Cairo::RefPtr<Cairo::Context>& cc);
	void drawTrains(const Cairo::RefPtr<Cairo::Context>& cc);
	void drawWantedEdgeCapacities(const Cairo::RefPtr<Cairo::Context>& cc);

	/// draw all of passengers at that point
	void drawPassengersAt(const geom::Point<float> point, const PassengerConstRefList& passengers, const Cairo::RefPtr<Cairo::Context>& cc);

	/// step time forward and force a redraw
	bool causeAnimationFrame();

	/// begin animating (does nothing if already doing so)
	void beginAnimating();
	/// stop animating (does nothing if already not)
	void stopAnimating();

private:
	friend class TrainsAreaData;

	/// forces a redraw of the entire trains area
	void forceRedraw();

	std::unique_lock<std::recursive_mutex> getScopedDrawingLock();
	util::ScopedLockAndData<bool,std::recursive_mutex> getIsAnimatingAndLock();

	/// bring the animation back to the start
	void resetAnimationTime();

	TrainsAreaData& data;

	sigc::connection animation_connection;
	std::recursive_mutex drawing_mutex;
};

} // end namespace graphics

#endif /* GRAPHICS__TRAINS_AREA_H */
