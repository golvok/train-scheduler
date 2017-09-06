
#include "trains_area.h++"

#include <graphics/utils.h++>
#include <util/graph_utils.h++>
#include <util/iteration_utils.h++>
#include <util/logging.h++>
#include <util/passenger.h++>
#include <util/utils.h++>

#include <cmath>
#include <string>
#include <vector>

namespace graphics {

using namespace geom;

TrainsArea::TrainsArea(TrainsAreaData& data)
	: data(data)
	, animation_connection()
	, drawing_mutex()
{
	data.setTrainsArea(this);
}

bool TrainsArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cc) {
	// guarantee that the data will not change between drawing calls
	auto sdl = data.getScopedDataLock();
	// guarantee that state information will not change between drawing calls
	auto sdrl = getScopedDrawingLock();

	centerOnTrackNework(cc);
	drawTrackNetwork(cc);
	drawTrains(cc);
	drawWantedEdgeCapacities(cc);

	data.notifyOfFrameDrawn();

	return true;
}

void TrainsArea::centerOnTrackNework(const Cairo::RefPtr<Cairo::Context>& cc) {
	auto sdl = data.getScopedDataLock(); // may get multiple things from the data
	auto tn = data.getTN();

	if (!tn) { return; }

	// get the size of this Widget
	const double alloc_width  = get_allocation().get_width();
	const double alloc_height = get_allocation().get_height();

	BoundBox<float> track_bb(tn->getVertexPosition(0),0.0,0.0); // a 0.0x0.0 rect at a valid vertex

	// figure out bounds. Expand track_bb to cover all vertices
	for (auto vi : make_iterable(boost::vertices(tn->g()))) {
		Point<float> v = tn->getVertexPosition(vi);

		if (v.x < track_bb.min_point().x) { track_bb.min_point().x = v.x; }
		if (v.y < track_bb.min_point().y) { track_bb.min_point().y = v.y; }
		if (v.x > track_bb.max_point().x) { track_bb.max_point().x = v.x; }
		if (v.y > track_bb.max_point().y) { track_bb.max_point().y = v.y; }
	}

	// padding is 20% of width, 20% of height, or 10.0. Whichever is larger
	const float padding = std::max({track_bb.get_width(),track_bb.get_height(),50.0f}) * 0.2;
	track_bb.min_point() -= Point<float>{padding,padding};
	track_bb.max_point() += Point<float>{padding,padding};

	enum class Orientation {
		HORIZ,VERT,
	};

	// we would like to zoom in/out enough so that we can see the complete bounds of the TN
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
	auto sim_handle = data.getSimulatorHandle();

	if (!tn) { return; }

	for (auto vi : make_iterable(boost::vertices(tn->g()))) {
		// draw vertex
		Point<float> v = tn->getVertexPosition(vi);
		const std::string& name = tn->getVertexName(vi);

		cc->set_source_rgb(0.0,0.0,0.0); // black

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

		cc->stroke();

		if (sim_handle && sim_handle.isPaused()) {
			// draw passengers where they currently are in the simulation
			drawPassengersAt(v, sim_handle.getPassengersAt(tn->getStationIDByVertexID(vi)), cc);
		} else {
			// draw passengers (if we have them) where they start.
			auto passengers = data.getPassengers();
			if (passengers) {
				PassengerConstRefList passengers_here;
				std::copy_if(passengers->begin(), passengers->end(), passengerRefListInserter(passengers_here), [&](auto& p) {
					return p.getEntryID() == vi;
				});
				drawPassengersAt(v, passengers_here, cc);
			}
		}
	}
}

void TrainsArea::drawTrains(const Cairo::RefPtr<Cairo::Context>& cc) {
	auto sdl = data.getScopedDataLock(); // may get multiple things from the data
	auto is_animating = getIsAnimatingAndLock();
	auto tn = data.getTN();
	auto schedule = data.getSchedule();
	auto sim_handle = data.getSimulatorHandle();

	if (!is_animating) { return; }
	if (!sim_handle) { return; }
	if (!sim_handle.isPaused()) { return; }

	for (const auto& train_id : sim_handle.getActiveTrains()) {
		const auto& path = schedule->getTrainRoute(train_id.getRouteID()).getPath();
		const auto& position_info = sim_handle.getTrainLocation(train_id);

		const auto prev_vertex_it = path.begin() + position_info.edge_number;
		const auto next_vertex_it = prev_vertex_it + 1;

		const auto prev_pt = tn->getVertexPosition(*prev_vertex_it);

		const auto p = [&]() {
			if (next_vertex_it == path.end()) {
				// at last vertex case
				return prev_pt;
			} else {
				// interpolate
				const auto next_pt = tn->getVertexPosition(*next_vertex_it);
				return prev_pt + (next_pt - prev_pt) * position_info.fraction_through_edge;
			}
		}();


		// draw ID
		cc->set_source_rgb(0,0,0);
		cc->set_font_size(4);
		cc->move_to(p.x,p.y-10);
		cc->save(); // would like to restore to unrotated matrix
		cc->rotate_degrees(45);
		cc->show_text(::util::stringify_through_stream(train_id).c_str());
		cc->stroke();
		cc->restore(); // restore to unrotated matrix

		// draw
		cc->set_source_rgb(0.0,0.0,1.0); // blue
		cc->arc(p.x,p.y, 0.5, 0, 2 * M_PI);
		cc->stroke();

		drawPassengersAt(p, sim_handle.getPassengersAt(train_id), cc);
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

	// draw the wanted capacity near the center of each edge
	for (auto edge_desc : make_iterable(edges(g))) {
		auto edge_index = tn->getEdgeIndex(edge_desc);
		auto source_point = tn->getVertexPosition(source(edge_desc,g));
		auto target_point = tn->getVertexPosition(target(edge_desc,g));

		auto middle_point = (source_point + target_point) / 2;

		cc->move_to(middle_point.x,middle_point.y+10);
		cc->save(); // would like to restore to unrotated matrix
		cc->rotate_degrees(45);
		cc->show_text(std::to_string((*wecs)[edge_index]).c_str());
		cc->restore(); // restore to unrotated matrix
	}
}

template<typename COLLECTION>
void TrainsArea::drawPassengersAt(const geom::Point<float> point, const COLLECTION& passengers, const Cairo::RefPtr<Cairo::Context>& cc) {
	auto sdl = data.getScopedDataLock(); // may get multiple things from the data
	auto is_animating = getIsAnimatingAndLock();
	auto tn = data.getTN();
	auto sim_handle = data.getSimulatorHandle();

	if (!tn) { return; }

	// draw a dot for each passenger
	int p_counter = 0;
	for (const Passenger& p : passengers) {

		if (!is_animating || !sim_handle || !sim_handle.isPaused() || sim_handle.getCurrentTime() == p.getStartTime()) {
			cc->set_source_rgb(0.0,1.0,0.0); // green
		} else if (sim_handle.getCurrentTime() > p.getStartTime()) {
			cc->set_source_rgb(1.0,1.0,0.0); // yellow
		} else {
			cc->set_source_rgb(1.0,0.0,0.0); // red
		}

		// draw this one a bit farther away than the last
		auto draw_point = point + (p_counter + 1)* make_point(0,-2);

		cc->move_to(draw_point.x,draw_point.y);
		cc->arc(draw_point.x,draw_point.y, 0.5, 0, 2 * M_PI);
	}
	cc->stroke();
}

bool TrainsArea::causeAnimationFrame() {
	auto is_animating = getIsAnimatingAndLock();

	bool retval = true;
	if (is_animating) {
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
}

} // end namespace graphics
