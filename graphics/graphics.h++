
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
 * The main API for making windows, and graphics elements.
 */
class Graphics {
	// PIMPL as to keep windowing & drawing dependencies in this directory
	class Impl;
	std::unique_ptr<Impl> impl;

	TrainsAreaData trains_area_data;
public:
	Graphics();

	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;


	/**
	 * Initializes the graphics library and makes the
	 * window(s) & graphics area(s)
	 */
	bool initialize();

	/**
	 * Blocks until the ever-present continue button is pressed.
	 * Returns immediately if no graphics are on
	 */
	void waitForPress();

	/**
	 * Get the data interface for the Trains Area.
	 * Main way of matking data availible to the Trains Area graphics.
	 */
	TrainsAreaData& trainsArea();
};

} // end namespace graphics

#endif /* GRAPHICS__GRAPHICS_H */