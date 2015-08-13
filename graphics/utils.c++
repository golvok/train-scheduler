#include "utils.h++"

namespace graphics {

namespace util {

using namespace geom;

void draw_arrow(const CairoRefPtr& cc, Point<double> tail, Point<double> tip) {
	Point<double> delta(tip.x - tail.x, tip.y - tail.y);
	const uint arrowLength = geom::magnitude(delta) * 0.05;

	double angle_of_line = std::atan2(delta.y, delta.x);

	const double phi = 35 * M_PI / 180;
	Point<double> p1(
		tip.x - arrowLength * std::cos(angle_of_line + phi),
		tip.y - arrowLength * std::sin(angle_of_line + phi)
	);

	const double phi2 = -35 * M_PI / 180;
	Point<double> p2(
		tip.x - arrowLength * std::cos(angle_of_line + phi2),
		tip.y - arrowLength * std::sin(angle_of_line + phi2)
	);

	draw_all(cc, {tail,tip,p1,p2,tip} );
}

void draw_box(const CairoRefPtr& cc, const BoundBox<double>& box) {
	draw_all(cc, {
		box.max_point(),
		Point<double>(box.minx(),box.maxy()),
		box.min_point(),
		Point<double>(box.maxx(),box.miny()),
		box.max_point()
	});
}

} // end namespace util

} // end namespace graphics
