
#ifndef UTIL__LOCATION_ID_HPP
#define UTIL__LOCATION_ID_HPP

#include <algo/scheduler.h++>
#include <util/track_network.h++>

struct LocationIDTag { static const uint DEFAULT_VALUE = -1; };
class LocationID : public ::util::ID<
	std::common_type_t<
		::algo::RouteID::IDType,
		StationID::IDType
	>,
	LocationIDTag
> {
private:
	LocationID(IDType val) : ID(val) { }
public:
	static const IDType TRAIN_FLAG = ((IDType)(-1)) & ~(((IDType)(-1)) >> 1);

	LocationID(::algo::RouteID tid) : ID(tid.getValue() | TRAIN_FLAG) { }
	LocationID(StationID sid) : ID(sid.getValue()) { }
	LocationID() : ID() { }

	bool isTrain() const { return (getValue() & TRAIN_FLAG) != 0; }
	bool isStation() const { return (getValue() & TRAIN_FLAG) == 0; }

	::algo::RouteID asRouteID() const {
		if (!isTrain()) { throw std::invalid_argument("Invalid Train id" + std::to_string(getValue())); }
		return ::util::make_id<::algo::RouteID>(getValue() & (~TRAIN_FLAG));
	}
	StationID asStationID() const {
		if (!isStation()) { throw std::invalid_argument("Invalid Station id" + std::to_string(getValue())); }
		return ::util::make_id<StationID>(getValue());
	}

	void print(std::ostream& os) const {
		if (getValue() == DEFAULT_VALUE) {
			os << "<DEFAULT>";
		} else if (isTrain()) {
			os << 't' << asRouteID().getValue();
		} else if (isStation()) {
			os << 's' << asStationID().getValue();
		}
	}
};

inline std::ostream& operator<<(std::ostream& os, LocationID loc) {
	loc.print(os);
	return os;
}

#endif /* UTIL__LOCATION_ID_HPP */
