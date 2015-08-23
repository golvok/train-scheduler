
#include "trains_area.h++"

#include <graphics/utils.h++>
#include <util/iteration_utils.h++>
#include <util/utils.h++>

#include <cmath>

namespace graphics {

using namespace geom;

namespace {

const uint INVALID_TIME = -1;

} // end anononymous namespace

TrainsArea::TrainsArea(TrainsAreaData& data)
	: data(data)
	, time(INVALID_TIME)
	, is_animating(false)
	, stop_animating(false)
{
	data.setTrainsArea(this);
}

bool TrainsArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cc) {
	auto sdl = data.getScopedDataLock();
	if (data.hasTN() == false) { return true; }
	centerOnTrackNework(cc);
	drawTrackNetwork(cc);
	drawTrains(cc);
	drawPassengers(cc);

	return true;
}

void TrainsArea::centerOnTrackNework(const Cairo::RefPtr<Cairo::Context>& cc) {
	if (data.hasTN() == false) { return; }
	auto& tn = data.getTN();

	const double alloc_width  = get_allocation().get_width();
	const double alloc_height = get_allocation().get_height();
	BoundBox<float> track_bb(tn.getVertexPosition(0),0.0,0.0);

	// figure out bounds.
	for (auto vi : make_iterable(boost::vertices(tn.g()))) {
		// draw vertex
		Point<float> v = tn.getVertexPosition(vi);

		if (v.x < track_bb.min_point().x) { track_bb.min_point().x = v.x; }
		if (v.y < track_bb.min_point().y) { track_bb.min_point().y = v.y; }
		if (v.x > track_bb.max_point().x) { track_bb.max_point().x = v.x; }
		if (v.y > track_bb.max_point().y) { track_bb.max_point().y = v.y; }
	}

	const float padding = std::max(track_bb.get_width(),track_bb.get_height()) * 0.2;
	track_bb.min_point() -= Point<float>{padding,padding};
	track_bb.max_point() += Point<float>{padding,padding};

	enum class Orientation {
		HORIZ,VERT,
	};

	auto scale = std::min(
		compare_with_tag(alloc_width/track_bb.get_width(),Orientation::HORIZ),
		compare_with_tag(alloc_height/track_bb.get_height(),Orientation::VERT)
	);

	// zoom in, without distortion
	cc->scale(scale.value(),scale.value());

	// put top minx corner at the origin
	cc->translate(-track_bb.minx(),-track_bb.miny());

	// center
	if (scale.id() == Orientation::HORIZ) {
		// need to shift down.
		cc->translate(0.0,(alloc_height/2-track_bb.get_height()*scale.value()/2)/scale.value());
	} else {
		// need to shift towards +x
		cc->translate((alloc_width/2-track_bb.get_width()*scale.value()/2)/scale.value(),0.0);
	}
}

void TrainsArea::drawTrackNetwork(const Cairo::RefPtr<Cairo::Context>& cc) {
	if (data.hasTN() == false) { return; }
	auto& tn = data.getTN();

	for (auto vi : make_iterable(boost::vertices(tn.g()))) {
		// draw vertex
		Point<float> v = tn.getVertexPosition(vi);
		const std::string& name = tn.getVertexName(vi);

		// draw a circle there
		cc->move_to(v.x,v.y);
		cc->arc(v.x,v.y, 2, 0, 2 * M_PI);

		// display the name
		cc->move_to(v.x,v.y-10);
		cc->show_text(name);

		// draw out edges
		for (auto outv : make_iterable(boost::out_edges(vi,tn.g()))) {
			auto outv_point = tn.getVertexPosition(boost::target(outv,tn.g()));
			graphics::util::draw_arrow(cc,v,outv_point);
		}
	}

	cc->stroke();
}

void TrainsArea::drawTrains(const Cairo::RefPtr<Cairo::Context>& cc) {
	if (data.hasTrains() == false ) { return; }
	if (is_animating == false) { return; }
	// draw trains...
	cc->stroke();
}

void TrainsArea::drawPassengers(const Cairo::RefPtr<Cairo::Context>& cc) {
	if (data.hasTN() == false) { return; dout << "no TN\n"; }
	if (data.hasPassengers() == false) { return;  dout << "no Passengers\n";}
	if (is_animating == false) { return;  dout << "not animating\n";}

	auto& tn = data.getTN();
	auto& psgrs = data.getPassengers();

	auto passenger_counts = tn.makeVertexMap<uint>(0);

	for (auto& p : psgrs) {
		if (time == p.getStartTime()) {
			cc->set_source_rgb(0.0,1.0,0.0); // green
		} else if (time > p.getStartTime()) {
			cc->set_source_rgb(1.0,1.0,0.0); // yellow
		} else {
			continue; // skip it
		}

		auto entry_id = p.getEntryId();
		auto& count_at_etry = passenger_counts[entry_id];
		count_at_etry += 1;
		auto draw_point = tn.getVertexPosition(entry_id);
		draw_point += count_at_etry * make_point(0,-2);

		cc->move_to(draw_point.x,draw_point.y);
		cc->arc(draw_point.x,draw_point.y, 0.5, 0, 2 * M_PI);
		cc->stroke();
	}
}

bool TrainsArea::causeAnimationFrame() {
	if (!is_animating) { return false; }
	time += 1;

	forceRedraw();

	if (stop_animating == true) {
		stop_animating = false;
		is_animating = false;
		return false;
	} else {
		return true;
	}
}

void TrainsArea::beginAnimating() {
	if (!is_animating && !stop_animating) {
		time = 0;
		is_animating = true;
		Glib::signal_timeout().connect( sigc::mem_fun(*this, &TrainsArea::causeAnimationFrame), 1000 );
	}
}

void TrainsArea::stopAnimating() {
	if (is_animating) {
		dout << "stop animating\n";
		stop_animating = true;
	}
}

void TrainsArea::forceRedraw() {
	// force redraw of entire area
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win) {
		Gdk::Rectangle r(0, 0, get_allocation().get_width(),get_allocation().get_height());
		win->invalidate_rect(r, false);
	}
}

} // end namespace graphics
