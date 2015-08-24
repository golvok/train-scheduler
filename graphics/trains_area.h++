
#ifndef GRAPHICS__TRAINS_AREA_H
#define GRAPHICS__TRAINS_AREA_H

#include "graphics.h++"
#include <graphics/windowing_includes.h++>

#include <cstdint>

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

	bool causeAnimationFrame();
	void beginAnimating();
	void stopAnimating();

private:
	friend class TrainsAreaData;

	void forceRedraw();
	bool isAnimating() { return animation_connection.connected(); }
	void resetAnimationTime();

	TrainsAreaData& data;
	uint time;

	sigc::connection animation_connection;
};

} // end namespace graphics

#endif /* GRAPHICS__TRAINS_AREA_H */
