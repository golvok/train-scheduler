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
	Glib::RefPtr<Gtk::Application> app;
	std::unique_ptr<Gtk::Window> window;
	std::unique_ptr<TrainsArea> ta;

	std::vector<std::unique_ptr<Gtk::Button>> buttons;
	std::unique_ptr<Gtk::Box> buttons_vbox;

	std::shared_ptr<util::SafeWaitForNotify> wait_for_press;

	bool is_initialized;

	TrainsAreaData& data;

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

		app_thread = std::thread([&] {
			int i = 0;
			// char* cs[2] = {nullptr};
			char** cc = nullptr;
			app = Gtk::Application::create(i, cc, "golvok.train-sch");

			window.reset(new Gtk::Window());
			ta.reset(new TrainsArea(data));

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

			// can't capture member variables directly...
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
