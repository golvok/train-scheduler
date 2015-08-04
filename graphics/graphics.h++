
#ifndef GRAPHICS__GRAPHICS_H
#define GRAPHICS__GRAPHICS_H

#include <memory>
#include <util/track_network.h++>

namespace graphics {

class TrainsAreaData;
class Graphics;

/**
 * Singleton getter for graphics
 */
Graphics& get();

/**
 * Data class for the trains area. The main way of geting data
 * into the graphics subsystem
 */
class TrainsAreaData {
	TrackNetwork* tn;
public:
	TrainsAreaData() : tn(nullptr) {}
	TrainsAreaData(const TrainsAreaData&) = delete;
	TrainsAreaData& operator=(const TrainsAreaData) = delete;

	TrackNetwork& getTN();
	bool hasTN();
	void clearTN();
	void setTN(TrackNetwork& new_tn);
};

/**
 * The main API for making windows, and graphics elements.
 */
class Graphics {
	// PIMPL as to keep windowing & drawing dependencies in this directory
	class Impl;
	std::unique_ptr<Impl> impl;
public:
	Graphics();

	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;


	/**
	 * Initializes the graphics library and makes the
	 * window(s) & graphics area(s)
	 */
	bool initialize();

	void waitForPress();

	TrainsAreaData& getTrainsAreaData();
};

} // end namespace graphics

#endif /* GRAPHICS__GRAPHICS_H */