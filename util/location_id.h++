
#ifndef UTIL__LOCATION_ID_HPP
#define UTIL__LOCATION_ID_HPP

#include <algo/scheduler.h++>
#include <util/track_network.h++>

struct LocationIdTag { static const uint DEFAULT_VALUE = -1; };
class LocationId : public ::util::ID<
	std::common_type_t<
		::algo::Train::TrainId::IdType,
		StationId::IdType
	>,
	LocationIdTag
> {
private:
	LocationId(IdType val) : ID(val) { }
public:
	static const IdType TRAIN_FLAG = ((IdType)(-1)) & ~(((IdType)(-1)) >> 1);

	LocationId(::algo::Train::TrainId tid) : ID(tid.getValue() | TRAIN_FLAG) { }
	LocationId(StationId sid) : ID(sid.getValue()) { }
	LocationId() : ID() { }

	bool isTrain() const { return (getValue() & TRAIN_FLAG) != 0; }
	bool isStation() const { return (getValue() & TRAIN_FLAG) == 0; }

	::algo::Train::TrainId asTrainId() const {
		if (!isTrain()) { throw std::invalid_argument("Invalid Train id" + std::to_string(getValue())); }
		return ::util::make_id<::algo::Train::TrainId>(getValue() & (~TRAIN_FLAG));
	}
	StationId asStationId() const {
		if (!isStation()) { throw std::invalid_argument("Invalid Station id" + std::to_string(getValue())); }
		return ::util::make_id<StationId>(getValue());
	}

	void print(std::ostream& os) const {
		if (getValue() == DEFAULT_VALUE) {
			os << "<DEFAULT>";
		} else if (isTrain()) {
			os << 't' << asTrainId().getValue();
		} else if (isStation()) {
			os << 's' << asStationId().getValue();
		}
	}
};

inline std::ostream& operator<<(std::ostream& os, LocationId loc) {
	loc.print(os);
	return os;
}

#endif /* UTIL__LOCATION_ID_HPP */
