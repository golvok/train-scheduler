
#ifndef GRAPHICS__TRAINS_AREA_H
#define GRAPHICS__TRAINS_AREA_H

#include "graphics.h++"
#include "windowing_includes.h++"

#include <cstdint>

namespace graphics {

class TrainsArea : public Gtk::DrawingArea {
	TrainsAreaData& data;
	uint time;
public:
	TrainsArea(TrainsAreaData& data);
	virtual ~TrainsArea() { }
protected:
	virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cc) override;
	bool forceRedraw();
};

} // end namespace graphics

#endif /* GRAPHICS__TRAINS_AREA_H */
