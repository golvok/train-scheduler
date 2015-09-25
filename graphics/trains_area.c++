
#include "trains_area.h++"

#include <graphics/utils.h++>
#include <util/graph_utils.h++>
#include <util/iteration_utils.h++>
#include <util/logging.h++>
#include <util/utils.h++>

#include <cmath>
#include <string>

namespace graphics {

using namespace geom;

namespace {

const uint INVALID_TIME = -1;

} // end anononymous namespace

TrainsArea::TrainsArea(TrainsAreaData& data)
	: data(data)
	, time(INVALID_TIME)
	, animation_connection()
	, drawing_mutex()
{
	data.setTrainsArea(this);
}

bool TrainsArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cc) {
	auto sdl = data.getScopedDataLock();
	auto sdrl = getScopedDrawingLock();

	centerOnTrackNework(cc);
	drawTrackNetwork(cc);
	drawTrains(cc);
	drawPassengers(cc);
	drawWantedEdgeCapacities(cc);

	return true;
}

void TrainsArea::centerOnTrackNework(const Cairo::RefPtr<Cairo::Context>& cc) {
	auto sdl = data.getScopedDataLock(); // may get multiple things from the data
	auto tn = data.getTN();

	if (!tn) { return; }

	const double alloc_width  = get_allocation().get_width();
	const double alloc_height = get_allocation().get_height();
	BoundBox<float> track_bb(tn->getVertexPosition(0),0.0,0.0);

	// figure out bounds.
	for (auto vi : make_iterable(boost::vertices(tn->g()))) {
		// draw vertex
		Point<float> v = tn->getVertexPosition(vi);

		if (v.x < track_bb.min_point().x) { track_bb.min_point().x = v.x; }
		if (v.y < track_bb.min_point().y) { track_bb.min_point().y = v.y; }
		if (v.x > track_bb.max_point().x) { track_bb.max_point().x = v.x; }
		if (v.y > track_bb.max_point().y) { track_bb.max_point().y = v.y; }
	}

	const float padding = std::max({track_bb.get_width(),track_bb.get_height(),50.0f}) * 0.2;
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
	auto sdl = data.getScopedDataLock(); // may get multiple things from the data
	auto tn = data.getTN();

	if (!tn) { return; }

	for (auto vi : make_iterable(boost::vertices(tn->g()))) {
		// draw vertex
		Point<float> v = tn->getVertexPosition(vi);
		const std::string& name = tn->getVertexName(vi);

		// draw a circle there
		cc->move_to(v.x,v.y);
		cc->arc(v.x,v.y, 2, 0, 2 * M_PI);

		// display the name
		cc->move_to(v.x,v.y-10);
		cc->show_text(name);

		// draw out edges
		for (auto outv : make_iterable(boost::out_edges(vi,tn->g()))) {
			auto outv_point = tn->getVertexPosition(boost::target(outv,tn->g()));
			graphics::util::draw_arrow(cc,v,outv_point);
		}
	}

	cc->stroke();
}

void TrainsArea::drawTrains(const Cairo::RefPtr<Cairo::Context>& cc) {
	auto sdl = data.getScopedDataLock(); // may get multiple things from the data
	auto is_animating = getIsAnimatingAndLock();
	auto schedule = data.getSchedule();
	auto tn = data.getTN();


	if (!is_animating) { return; }
	if (!tn) { return; }
	if (!schedule) { return; }

	auto& g = tn->g();

	cc->set_source_rgb(0.0,0.0,1.0); // blue

	for (auto& train : schedule->getTrains()) {
		auto& route = train.getRoute();

		TrainsArea::Time time_in_nework = time - train.getDepartureTime();
		float time_until_prev_vertex = 0;

		TrackNetwork::ID prev_vertex(-1);
		for (TrackNetwork::ID id : route) {
			if (prev_vertex == TrackNetwork::ID(-1)) {
				prev_vertex = std::move(id);
				continue; // skip first one.
			}

			auto edge_desc = boost::edge(prev_vertex,id,g).first;

			float additional_time_to_next_vertex =
				boost::get(&TrackNetwork::EdgeProperties::weight,g,edge_desc) / train.getSpeed();

			if ((time_until_prev_vertex + additional_time_to_next_vertex) >= time_in_nework) {
				auto prev_pt = tn->getVertexPosition(prev_vertex);
				auto next_pt = tn->getVertexPosition(id);

				// interpolate
				auto fraction_distance_travelled = ((time_in_nework - time_until_prev_vertex)/(additional_time_to_next_vertex));
				auto offset_to_next = (next_pt - prev_pt) * fraction_distance_travelled;
				auto p = prev_pt + offset_to_next;

				cc->arc(p.x,p.y, 0.5, 0, 2 * M_PI);
				cc->stroke();
				break;
			}

			prev_vertex = std::move(id);
			time_until_prev_vertex += additional_time_to_next_vertex;
		}
	}

}

void TrainsArea::drawPassengers(const Cairo::RefPtr<Cairo::Context>& cc) {
	auto sdl = data.getScopedDataLock(); // may get multiple things from the data
	auto is_animating = getIsAnimatingAndLock();
	auto tn = data.getTN();
	auto psgrs = data.getPassengers();

	if (!is_animating) { return; }
	if (!tn) { return; }
	if (!psgrs) { return; }

	auto passenger_counts = ::util::makeVertexMap<uint>(tn->g(),0);

	for (auto& p : *psgrs) {
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
		auto draw_point = tn->getVertexPosition(entry_id);
		draw_point += count_at_etry * make_point(0,-2);

		cc->move_to(draw_point.x,draw_point.y);
		cc->arc(draw_point.x,draw_point.y, 0.5, 0, 2 * M_PI);
		cc->stroke();
	}
}

void TrainsArea::drawWantedEdgeCapacities(const Cairo::RefPtr<Cairo::Context>& cc) {
	auto sdl = data.getScopedDataLock(); // may get multiple things from the data
	auto tn = data.getTN();
	auto wecs = data.getWantedEdgeCapacities();

	if (!tn) { return; }
	if (!wecs) { return; }

	auto& g = tn->g();

	cc->set_source_rgb(0,0,0); // black
	cc->set_font_size(6);

	for (auto edge_desc : make_iterable(edges(g))) {
		auto edge_index = tn->getEdgeIndex(edge_desc);
		auto source_point = tn->getVertexPosition(source(edge_desc,g));
		auto target_point = tn->getVertexPosition(target(edge_desc,g));

		auto middle_point = (source_point + target_point) / 2;

		cc->move_to(middle_point.x,middle_point.y+10);
		cc->save();
		cc->rotate_degrees(45);
		cc->show_text(std::to_string((*wecs)[edge_index]).c_str());
		cc->restore();
	}
}

bool TrainsArea::causeAnimationFrame() {
	auto is_animating = getIsAnimatingAndLock();

	bool retval = true;
	if (is_animating) {
		time += 1;
		retval = true;
	} else {
		retval = false;
	}

	forceRedraw();

	return retval;
}

void TrainsArea::beginAnimating() {
	auto is_animating = getIsAnimatingAndLock();

	if (!is_animating) {
		animation_connection = Glib::signal_timeout().connect(
			sigc::mem_fun(*this, &TrainsArea::causeAnimationFrame),
			1000
		);
	}
}

void TrainsArea::stopAnimating() {
	auto is_animating = getIsAnimatingAndLock();

	if (is_animating) {
		animation_connection.disconnect();
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

std::unique_lock<std::recursive_mutex> TrainsArea::getScopedDrawingLock() {
	return std::unique_lock<std::recursive_mutex>(drawing_mutex);
}

::util::ScopedLockAndData<bool,std::recursive_mutex> TrainsArea::getIsAnimatingAndLock() {
	return ::util::ScopedLockAndData<bool,std::recursive_mutex>(
		animation_connection.connected(),
		getScopedDrawingLock()
	);
}

void TrainsArea::resetAnimationTime() {
	getScopedDrawingLock();
	time = 0;
}

} // end namespace graphics
