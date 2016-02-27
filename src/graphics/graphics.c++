#include "graphics.h++"

#include <graphics/windowing_includes.h++>
#include <graphics/trains_area.h++>
#include <util/thread_utils.h++>

#include <cassert>
#include <thread>

namespace graphics {

Graphics& get() {
	static Graphics singleton;
	return singleton;
}

class Graphics::Impl {
	/// Gtk "app"
	Glib::RefPtr<Gtk::Application> app;
	/// main window - needs to be pointer: constructor fails if app is not constructed
	std::unique_ptr<Gtk::Window> window;
	/// trains area in main window - also needs to be pointer
	std::unique_ptr<TrainsArea> ta;

	/// the list of buttons present at the side of the main window - also needs to be a pointer
	std::vector<std::unique_ptr<Gtk::Button>> buttons;
	/// the vbox containing them - also needs to be a pointer
	std::unique_ptr<Gtk::Box> buttons_vbox;

	/// cond var used for communicating presses from the GUI thread
	/// is shared between the rest of the program and the GUI thread
	std::shared_ptr<util::SafeWaitForNotify> wait_for_press;

	/// prevents another call to initialize() while running.
	bool is_initialized;

	/// data for the trains area
	TrainsAreaData& data;

	/// the GUI thread
	std::thread app_thread;
public:
	Impl(TrainsAreaData& tad)
		: app()
		, window()
		, ta()
		, buttons()
		, buttons_vbox()
		, wait_for_press(new util::SafeWaitForNotify())
		, is_initialized(false)
		, data(tad)
		, app_thread()
	{
		initialize();
	}

	Impl(const Impl&) = delete;
	Impl& operator=(const Impl&) = delete;

	~Impl() {
		window->close(); // should cause app->run to return
		if (app_thread.joinable()) {
			app_thread.join();
		}
	}

	bool initialize() {
		assert(is_initialized == false);
		is_initialized = true;

		// start the GUI thread
		app_thread = std::thread([&] {

			// create the "app"
			int i = 0;
			char** cc = nullptr;
			app = Gtk::Application::create(i, cc, "golvok.train-sch");

			// create & setup the window
			window.reset(new Gtk::Window());
			ta.reset(new TrainsArea(data));

			window->set_default_size(800,600);
			window->set_title("Train Scheduler");

			// this will be the top level
			Gtk::Box draw_and_buttons_hbox(Gtk::Orientation::ORIENTATION_HORIZONTAL);
			// this will hold all the buttons
			buttons_vbox.reset(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL));

			// make the button sidebar a different colour
			buttons_vbox->override_background_color(Gdk::RGBA("#AAA"));

			// put trains area at the left (will expand to fill remaining space)
			draw_and_buttons_hbox.pack_start(*ta,true,true);
			// put button container at the right (will remain original size)
			draw_and_buttons_hbox.pack_end(*buttons_vbox,false,false);

			window->add(draw_and_buttons_hbox);

			ta->show();
			draw_and_buttons_hbox.show();
			buttons_vbox->show();

			// can't capture member variables directly...
			// also, this probably creates a race condition...
			auto& tmp_wait_for_press = wait_for_press;
			addButton("continue", [tmp_wait_for_press] {
				tmp_wait_for_press->notify_all();
			});

			app->run(*window);

			is_initialized = false;
			wait_for_press->notify_all();
		});

		return true;
	}

	void waitForPress() {
		if (is_initialized == false) { return; }
		wait_for_press->wait();
		// careful - threads may be here after this is destructed
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
	, trains_area_data()
{ }

bool Graphics::initialize() {
	// create a new Impl if none exists, or destruct then construct the old one
	if (!impl) {
		impl.reset(new Graphics::Impl(trains_area_data));
	} else {
		impl->~Impl();
		new (&(*impl)) Graphics::Impl(trains_area_data);
	}
	return true;
}

void Graphics::waitForPress() { if (impl) { impl->waitForPress(); } }

TrainsAreaData& Graphics::trainsArea() { return trains_area_data; }

} // end namespace graphics
