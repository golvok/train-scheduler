#include "graphics.h++"

#include "windowing_includes.h++"
#include "trains_area.h++"

#include <cassert>
#include <thread>
#include <condition_variable>

namespace graphics {

Graphics& get() {
	static Graphics singleton;
	return singleton;
}

TrackNetwork& TrainsAreaData::getTN() { assert(tn != nullptr); return *tn; }
bool TrainsAreaData::hasTN() { return tn != nullptr; }
void TrainsAreaData::clearTN() { tn = nullptr; }
void TrainsAreaData::setTN(TrackNetwork& new_tn) {
	assert(&new_tn != nullptr);
	clearTN();
	tn = &new_tn;
}

class Graphics::Impl {
	Glib::RefPtr<Gtk::Application> app;
	std::unique_ptr<Gtk::Window> window;
	std::unique_ptr<TrainsArea> ta;

	std::vector<std::unique_ptr<Gtk::Button>> buttons;
	std::unique_ptr<Gtk::Box> buttons_vbox;

	std::condition_variable waitForPress_cv;
	std::mutex waitForPress_lk;

	bool is_initialized;

	TrainsAreaData tad;
public:
	Impl()
		: app()
		, window()
		, ta()
		, buttons()
		, buttons_vbox()
		, waitForPress_cv()
		, waitForPress_lk()
		, is_initialized(false)
		, tad()
	{ }

	Impl(const Impl&) = delete;
	Impl& operator=(const Impl&) = delete;

	bool initialize() {
		assert(is_initialized == false);
		is_initialized = true;

		std::thread([&](){
			int i = 0;
			// char* cs[2] = {nullptr};
			char** cc = nullptr;
			app = Gtk::Application::create(i, cc, "golvok.train-sch");

			window.reset(new Gtk::Window());
			ta.reset(new TrainsArea(tad));

			window->set_default_size(800,600);
			window->set_title("Train Scheduler");

			Gtk::Box draw_and_buttons_hbox(Gtk::Orientation::ORIENTATION_HORIZONTAL);
			buttons_vbox.reset(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL));

			buttons_vbox->override_background_color(Gdk::RGBA("#AAA"));

			draw_and_buttons_hbox.pack_start(*ta,true,true);
			draw_and_buttons_hbox.pack_end(*buttons_vbox,false,false);

			window->add(draw_and_buttons_hbox);
			
			ta->show();
			draw_and_buttons_hbox.show();
			buttons_vbox->show();

			addButton("continue",[&]{
				waitForPress_cv.notify_all();
			});

			app->run(*window);

			is_initialized = false;
			waitForPress_cv.notify_all();
		}).detach();

		return true;
	}

	void waitForPress() {
		if (is_initialized == false) { return; }
		std::unique_lock<std::mutex> ul(waitForPress_lk);
		waitForPress_cv.wait(ul);
	}

	TrainsAreaData& getTrainsAreaData() {
		return tad;
	}

	template<typename FUNC>
	size_t addButton(std::string text, FUNC f) {
		buttons.emplace_back(new Gtk::Button(text));
		auto& button = *buttons.back();
		button.signal_clicked().connect(f);
		buttons_vbox->pack_end(button,false,false);
		button.show();
		return buttons.size() - 1;
	}
};

Graphics::Graphics()
	: impl(nullptr)
{ }

bool Graphics::initialize() {
	if (!impl) {
		impl.reset(new Graphics::Impl());
	} else {
		impl->~Impl();
		new (&(*impl)) Graphics::Impl();
	}
	return impl->initialize();
}

void Graphics::waitForPress() { impl->waitForPress(); }

TrainsAreaData& Graphics::getTrainsAreaData() { return impl->getTrainsAreaData(); }

} // end namespace graphics
