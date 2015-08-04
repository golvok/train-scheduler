
#include "trains_area.h++"

#include <util/utils.h++>
#include <cmath>

namespace graphics {

namespace {

const uint INVALID_TIME = -1;

} // end anononymous namespace

TrainsArea::TrainsArea(TrainsAreaData& data)
	: data(data)
	, time(INVALID_TIME)
{
	// set refresh rate for this
	Glib::signal_timeout().connect( sigc::mem_fun(*this, &TrainsArea::forceRedraw), 1000 );
}

/**
 * Draws the track & trains
 */
bool TrainsArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cc) {
	(void)cc;

	return true;
}

bool TrainsArea::forceRedraw() {
	time += 1;
	// force redraw of entire area
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win) {
		Gdk::Rectangle r(0, 0, get_allocation().get_width(),get_allocation().get_height());
		win->invalidate_rect(r, false);
	}
	return true;
}

}
