
#ifndef UTIL__LOCATION_ID_HPP
#define UTIL__LOCATION_ID_HPP

#include <algo/train_route.h++>
#include <util/track_network.h++>

struct LocationIDTag { static const uint DEFAULT_VALUE = -1; };
class LocationID : public ::util::ID<
	std::common_type_t<
		::algo::TrainID::IDType,
		StationID::IDType
	>,
	LocationIDTag
> {
private:
	LocationID(IDType val) : ID(val) { }
public:
	static const IDType TRAIN_FLAG = ((IDType)(-1)) & ~(((IDType)(-1)) >> 1);

	LocationID(::algo::TrainID tid) : ID(tid.getValue() | TRAIN_FLAG) { }
	LocationID(StationID sid) : ID(sid.getValue()) { }
	LocationID() : ID() { }

	bool isTrain() const { return (getValue() & TRAIN_FLAG) != 0; }
	bool isStation() const { return (getValue() & TRAIN_FLAG) == 0; }

	::algo::TrainID asTrainID() const {
		if (!isTrain()) { throw std::invalid_argument("Invalid Train id" + std::to_string(getValue())); }
		return ::algo::TrainID(getValue() & (~TRAIN_FLAG));
	}
	StationID asStationID() const {
		if (!isStation()) { throw std::invalid_argument("Invalid Station id" + std::to_string(getValue())); }
		return ::util::make_id<StationID>(getValue());
	}

	void print(std::ostream& os) const {
		if (getValue() == DEFAULT_VALUE) {
			os << "<DEFAULT_LOCATION>";
		} else if (isTrain()) {
			asTrainID().print(os);
		} else if (isStation()) {
			os << 's' << asStationID().getValue();
		}
	}

	bool operator==(const ::algo::TrainID& tid) const {
		return isTrain() && asTrainID() == tid;
	}
	bool operator==(const StationID& sid) const {
		return isStation() && asStationID() == sid;
	}
};

// ==, with lid on the right
inline bool operator==(const ::algo::TrainID& tid, const LocationID& lid) { return lid == tid; }
inline bool operator==(const StationID& sid, const LocationID& lid) { return lid == sid; }


inline std::ostream& operator<<(std::ostream& os, const LocationID& loc) {
	loc.print(os);
	return os;
}

namespace std {
	template<>
	struct hash<LocationID> {
		size_t operator()(const LocationID& id) const {
			return std::hash<decltype(id.getValue())>()(id.getValue());
		}
	};
}

#endif /* UTIL__LOCATION_ID_HPP */
