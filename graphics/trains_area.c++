
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
	if (time == INVALID_TIME) { return true; }

	const uint padding = 10;

	const int width  = get_allocation().get_width()  - padding*2;
	const int height = get_allocation().get_height() - padding*2;

	std::vector<TrackNetwork::ID> vertices;

	for (auto vi : make_iterable(boost::vertices(data.getTN().g()))) {
		vertices.push_back(vi);
	}

	std::sort(vertices.begin(), vertices.end(), [&](auto& lhs, auto& rhs) {
		return data.getTN().getVertexName(lhs) < data.getTN().getVertexName(rhs);
	});

	float vertex_spacing = (float)width/vertices.size();

	const int y = 0 + padding + height/2; // center
	float x = padding;

	cc->move_to(x,y);

	for (auto& id : vertices) {
		cc->line_to(x,y);
		cc->arc(x,y, 2, 0, 2 * M_PI);

		cc->move_to(x,y-10);
		cc->show_text(data.getTN().getVertexName(id));
		cc->move_to(x,y);

		x += vertex_spacing;
	}

	cc->stroke();

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
