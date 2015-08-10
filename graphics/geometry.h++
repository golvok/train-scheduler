
#ifndef GRAPHICS__GEOMETRY_H
#define GRAPHICS__GEOMETRY_H

#include <ostream>
#include <cmath>

namespace geom {

/**
 * A point datatype.
 */
template<typename PRECISION>
struct Point {
	PRECISION x;
	PRECISION y;

	Point() : x(0), y(0) { }
	template<typename THEIR_PRECISION>
	Point(const Point<THEIR_PRECISION>& src) : Point(src.x,src.y) { }
	Point(PRECISION x, PRECISION y) : x(x), y(y) { }

	void set(PRECISION x, PRECISION y) {
		this->x = x;
		this->y = y;
	}
	void set(const Point& src) {
		x = src.x;
		y = src.y;
	}

	/**
	 * Behaves like a 2 argument plusequals.
	 */
	void offset(PRECISION x, PRECISION y) {
		x += x;
		y += y;
	}

	/**
	 * These add the given point to this point in a
	 * componentwise fashion, ie x = x + rhs.x
	 *
	 * Naturally, {+,-} don't modify and {+,-}= do.
	 */
	Point operator+ (const Point& rhs) const {
		Point result = *this;
		result += rhs;
		return result;
	}
	Point operator- (const Point& rhs) const {
		Point result = *this;
		result -= rhs;
		return result;
	}
	Point operator* (PRECISION rhs) const {
		Point result = *this;
		result *= rhs;
		return result;
	}
	Point& operator+= (const Point& rhs) {
		this->x += rhs.x;
		this->y += rhs.y;
		return *this;
	}
	Point& operator-= (const Point& rhs) {
		this->x -= rhs.x;
		this->y -= rhs.y;
		return *this;
	}
	Point& operator*= (PRECISION rhs) {
		this->x *= rhs;
		this->y *= rhs;
		return *this;
	}

	/**
	 * Assign that point to this one - copy the components
	 */
	Point& operator= (const Point& src) {
		this->x = src.x;
		this->y = src.y;
		return *this;
	}

};

/**
 * constructor helper - will chose a type parameter for Point
 * that will be at least as precise as x and y
 */
template<typename PRECISION1, typename PRECISION2>
auto make_point(PRECISION1 x, PRECISION2 y) -> Point<decltype(x+y)> {
	return {x,y};
}

const int POSITIVE_DOT_PRODUCT = 0;
const int NEGATIVE_DOT_PRODUCT = 1;

template<typename PRECISION, typename PRECISION2>
auto deltaX(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return p2.x - p1.x;
}
template<typename PRECISION, typename PRECISION2>
auto deltaY(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return p2.y - p1.y;
}
template<typename PRECISION, typename PRECISION2>
Point<PRECISION> delta(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return {deltaX(p1, p2), deltaY(p1, p2)};
}
template<typename PRECISION, typename PRECISION2>
auto multiply(Point<PRECISION> p, PRECISION2 constant) {
	return make_point(p.x * constant, p.y * constant);
}
template<typename PRECISION, typename PRECISION2>
auto divide(Point<PRECISION> p, PRECISION2 constant) {
	return make_point(p.x / constant, p.y / constant);
}
template<typename PRECISION, typename PRECISION2>
auto add(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return make_point(p1.x + p2.x, p1.y + p2.y);
}
template<typename PRECISION, typename PRECISION2>
auto distance(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return sqrt(pow(deltaX(p1, p2), 2) + pow(deltaY(p1, p2), 2));
}
template<typename PRECISION>
auto magnitudeSquared(Point<PRECISION> p) {
	return pow(p.x, 2) + pow(p.y, 2);
}
template<typename PRECISION>
auto magnitude(Point<PRECISION> p) {
	return sqrt(magnitudeSquared(p));
}
template<typename PRECISION>
auto unit(Point<PRECISION> p) {
	return divide(p, (PRECISION) magnitude(p));
}
template<typename PRECISION, typename PRECISION2>
auto dotProduct(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return p1.x * p2.x + p1.y * p2.y;
}
template<typename PRECISION>
Point<PRECISION> getPerpindular(Point<PRECISION> p) {
	return {p.y,-p.x};
}
template<typename PRECISION, typename PRECISION2>
auto project(Point<PRECISION> source, Point<PRECISION2> wall) {
	return multiply(wall, dotProduct(wall, source) / magnitudeSquared(wall));
}
template<typename PRECISION, typename POINT_TYPE>
POINT_TYPE& farthestPoint(Point<PRECISION> p, const std::vector<POINT_TYPE>& tests) {
	int farthestIndex = 0;
	auto farthestDistance = distancef(p, tests[0]);
	for (int i = 1; i < tests.size(); ++i) {
		auto distance = distancef(p, tests[i]);
		if (farthestDistance < distance) {
			farthestIndex = i;
			farthestDistance = distance;
		}
	}
	return tests[farthestIndex];
}
template<typename PRECISION, typename PRECISION2, typename POINT_TYPE>
std::array<POINT_TYPE*,2> farthestFromLineWithSides(Point<PRECISION> p, Point<PRECISION2> q, std::vector<POINT_TYPE>& tests) {
	auto line = delta(p, q);
	std::array<POINT_TYPE,2> farthests;
	std::array<decltype(magnitude(perpindictularDeltaVectorToLine(line, delta(p, tests[0])))),2> farthestDistance;
	for (int i = 0; i < tests.size(); ++i) {
		auto relativeToP = delta(p, tests[i]);
		auto distance = magnitude(perpindictularDeltaVectorToLine(line, relativeToP));
		int dotProductSign = dotProduct(line, relativeToP) < 0 ? NEGATIVE_DOT_PRODUCT : POSITIVE_DOT_PRODUCT;
		if (farthestDistance[dotProductSign] < distance) {
			farthests[dotProductSign] = &tests[i];
			farthestDistance[dotProductSign] = distance;
		}
	}
	return farthests;
}
template<typename PRECISION, typename PRECISION2, typename PRECISION3>
auto distanceToLine(Point<PRECISION> onLine_1, Point<PRECISION2> onLine_2, Point<PRECISION3> p) {
	return magnitude(perpindictularDeltaVectorToLine(onLine_1, onLine_2, p));
}
template<typename PRECISION, typename PRECISION2>
auto perpindictularDeltaVectorToLine(Point<PRECISION> onLine_1, Point<PRECISION2> onLine_2, Point<PRECISION> p) {
	return perpindictularDeltaVectorToLine(delta(onLine_1, onLine_2), delta(p, onLine_1));
}
template<typename PRECISION, typename PRECISION2>
auto perpindictularDeltaVectorToLine(Point<PRECISION> direction, Point<PRECISION2> p) {
	return delta(p, project(p, direction));
}

template<typename PRECISION>
auto operator*(PRECISION lhs, const Point<PRECISION>& rhs) {
	return rhs*lhs;
}

template<typename PRECISION>
std::ostream& operator<<(std::ostream& os, const Point<PRECISION>& p) {
	os << '{' << p.x << ',' << p.y << '}';
	return os;
}

/**
 * Represents a rectangle, used as a bounding box.
 */
template<typename PRECISION>
class BoundBox {
public:
	using point_type = Point<PRECISION>;

private:
	point_type minpoint;
	point_type maxpoint;

public:
	BoundBox()
		: minpoint()
		, maxpoint()
	{ }

	template<typename THEIR_PRECISION>
	BoundBox(const BoundBox<THEIR_PRECISION>& src)
		: minpoint(src.min_point())
		, maxpoint(src.max_point())
	{ }

	BoundBox(PRECISION minx, PRECISION miny, PRECISION maxx, PRECISION maxy)
		: minpoint(minx,miny)
		, maxpoint(maxx,maxy)
	{ }

	template<typename THEIR_PRECISION, typename THEIR_PRECISION2>
	BoundBox(const Point<THEIR_PRECISION>& minpoint, const Point<THEIR_PRECISION2>& maxpoint)
		: minpoint(minpoint)
		, maxpoint(maxpoint)
	{ }

	template<typename THEIR_PRECISION, typename THEIR_PRECISION2, typename THEIR_PRECISION3>
	BoundBox(const Point<THEIR_PRECISION>& minpoint, THEIR_PRECISION2 width, THEIR_PRECISION3 height)
		: minpoint(minpoint)
		, maxpoint(minpoint)
	{
		maxpoint.offset(width, height);
	}

	/**
	 * These return their respective edge/point's location
	 */
	const PRECISION& minx() const { return min_point().x; }
	const PRECISION& maxx() const { return max_point().x; }
	const PRECISION& miny() const { return min_point().y; }
	const PRECISION& maxy() const { return max_point().y; }
	PRECISION& minx() { return min_point().x; }
	PRECISION& maxx() { return max_point().x; }
	PRECISION& miny() { return min_point().y; }
	PRECISION& maxy() { return max_point().y; }

	const point_type& min_point() const { return minpoint; }
	const point_type& max_point() const { return maxpoint; }
	point_type& min_point() { return minpoint; }
	point_type& max_point() { return maxpoint; }

	/**
	 * Calculate and return the center
	 */
	PRECISION get_xcenter() const { return (maxx() + minx()) / 2; }
	PRECISION get_ycenter() const { return (maxy() + miny()) / 2; }
	point_type get_center() const { return point_type(get_xcenter(), get_ycenter()); }

	/**
	 * Calculate and return the width/height
	 * ie. maxx/maxy - minx/miny respectively.
	 */
	PRECISION get_width()  const { return maxx() - minx(); }
	PRECISION get_height() const { return maxy() - miny(); }

	/**
	 * These behave like the plusequal operator
	 * They add their x and y values to all corners
	 */
	void offset(const point_type& make_relative_to) {
		this->minpoint += make_relative_to;
		this->maxpoint += make_relative_to;
	}

	void offset(PRECISION by_x, PRECISION by_y) {
		this->minpoint.offset(by_x, by_y);
		this->maxpoint.offset(by_x, by_y);
	}

	/**
	 * Does the given point coinside with this bbox?
	 * Points on the edges or corners are included.
	 */
	bool intersects(const point_type& test_pt) const {
		return intersects(test_pt.x, test_pt.y);
	}

	bool intersects(PRECISION x, PRECISION y) const {
		if (x < minx() || maxx() < x || y < miny() || maxy() < y) {
			return false;
		} else {
			return true;
		}
	}

	/**
	 * Calculate and return the area of this rectangle.
	 */
	PRECISION area() const {
		return std::abs(get_width() * get_height());
	}

	/**
	 * These add the given point to this bbox - they
	 * offset each corner by this point. Usful for calculating
	 * the location of a box in a higher scope, or for moving
	 * it around as part of a calculation
	 *
	 * Naturally, the {+,-} don't modify and the {+,-}= do.
	 */
	BoundBox operator+ (const point_type& rhs) const {
		BoundBox result = *this;
		result.offset(rhs);
		return result;
	}

	BoundBox operator- (const point_type& rhs) const {
		BoundBox result = *this;
		result.offset(point_type(-rhs.x, -rhs.y));
		return result;
	}

	BoundBox& operator+= (const point_type& rhs) {
		this->offset(rhs);
		return *this;
	}

	BoundBox& operator-= (const point_type& rhs) {
		this->offset(point_type(-rhs.x, -rhs.y));
		return *this;
	}

	/**
	 * Assign that box to this one - copy it's minx, maxx, miny, and maxy.
	 */
	BoundBox& operator= (const BoundBox& src) {
		this->min_point() = src.min_point();
		this->max_point() = src.max_point();
		return *this;
	}
};

} // end namespace geom

#endif /* GRAPHICS__GEOMETRY_H */
