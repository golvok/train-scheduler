
#ifndef GRAPHICS__GRAPHICS_H
#define GRAPHICS__GRAPHICS_H

#include <graphics/trains_area_data.h++>

#include <memory>

namespace graphics {

class Graphics;
class TrainsArea;

/**
 * Singleton getter for graphics
 */
Graphics& get();

/**
 * The main API for making windows and graphics elements.
 */
class Graphics {
	// PIMPL to keep windowing & drawing dependencies contained
	class Impl;
	std::unique_ptr<Impl> impl;

	// this is here (as opposed to being in the Impl) so that it always exists
	// and can be written to.
	TrainsAreaData trains_area_data;
public:
	Graphics();

	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;


	/**
	 * Initializes the graphics library(ies) and makes the
	 * initial window(s) & graphics area(s)
	 */
	bool initialize();

	/**
	 * Blocks until the ever-present continue button is pressed.
	 * Returns immediately if no graphics, or if the window with the continue
	 * button has been closed.
	 */
	void waitForPress();

	/**
	 * Get the data interface for the Trains Area.
	 * The intended way of making data availible to the Trains Area graphics.
	 */
	TrainsAreaData& trainsArea();
};

} // end namespace graphics

#endif /* GRAPHICS__GRAPHICS_H */