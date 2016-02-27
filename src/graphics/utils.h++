
#ifndef GRAPHICS__UTILS_H
#define GRAPHICS__UTILS_H

#include <graphics/windowing_includes.h++>
#include <graphics/graphics.h++>
#include <graphics/geometry.h++>

namespace graphics {

typedef Cairo::RefPtr<Cairo::Context> CairoRefPtr;

namespace util {

template<typename FUNC>
void point_scope(const CairoRefPtr& cc, FUNC f) {
	double oldx;
	double oldy;
	cairo_get_current_point(cc->cobj(),&oldx,&oldy);
	f();
	cc->move_to(oldx,oldy);
}

void draw_arrow(const CairoRefPtr& cc, geom::Point<double> start, geom::Point<double> end);

void draw_box(const CairoRefPtr& cc, const geom::BoundBox<double>& box);

template<typename T = geom::Point<double>, typename CONTAINER = std::initializer_list<T>>
void draw_all(const CairoRefPtr& cc, const CONTAINER& c) {
	if (std::begin(c) == std::end(c)) { return; }
	cc->move_to(std::begin(c)->x, std::begin(c)->y);
	for (auto& point : c) {
		cc->line_to(point.x,point.y);
	}
}

} // end namespace util

} // end namespace graphics

#endif /* GRAPHICS__UTILS_H */
