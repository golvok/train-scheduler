
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
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cc) override;
	void centerOnTrackNework(const Cairo::RefPtr<Cairo::Context>& cc);
	void drawTrackNetwork(const Cairo::RefPtr<Cairo::Context>& cc);
	void drawTrains(const Cairo::RefPtr<Cairo::Context>& cc);
	void drawPassengers(const Cairo::RefPtr<Cairo::Context>& cc);
	void drawWantedEdgeCapacities(const Cairo::RefPtr<Cairo::Context>& cc);

	bool causeAnimationFrame();
	void beginAnimating();
	void stopAnimating();

private:
	friend class TrainsAreaData;

	void forceRedraw();
	std::unique_lock<std::recursive_mutex> getScopedDrawingLock();
	util::ScopedLockAndData<bool,std::recursive_mutex> getIsAnimatingAndLock();
	void resetAnimationTime();

	TrainsAreaData& data;
	uint time;

	sigc::connection animation_connection;
	std::recursive_mutex drawing_mutex;
};

} // end namespace graphics

#endif /* GRAPHICS__TRAINS_AREA_H */
