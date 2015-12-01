
#ifndef UTIL__PASSENGER_H
#define UTIL__PASSENGER_H

#include <util/track_network.h++>
#include <util/utils.h++>

#include <iosfwd>

struct PassengerIDTag { const static uint DEFAULT_VALUE = -1; };
using PassengerID = ::util::ID<uint,PassengerIDTag>;

class Passenger {
private:
	std::string name;
	PassengerID id;
	TrackNetwork::ID entry_id;
	TrackNetwork::ID exit_id;
	uint start_time;

public:
	Passenger(
		const std::string& name,
		PassengerID id,
		TrackNetwork::ID entry_id,
		TrackNetwork::ID exit_id,
		uint start_time
	)
		: name(name)
		, id(id)
		, entry_id(entry_id)
		, exit_id(exit_id)
		, start_time(start_time)
	{}

	PassengerID getID() const { return id; }
	const std::string& getName() const { return name; }
	TrackNetwork::ID getEntryID() const { return entry_id; }
	TrackNetwork::ID getExitID() const { return exit_id; }
	uint getStartTime() const { return start_time; }

	bool operator==(const Passenger& rhs) const {
		return id == rhs.id;
	}
};

using PassengerIDList = std::vector<PassengerID>;
using PassengerList = std::vector<Passenger>;

// std::ostream& operator<<(std::ostresam& os, const Passenger& p);
std::ostream& operator<<(std::ostream& os, std::pair<const Passenger&,const TrackNetwork&> pair);

namespace std {
	template<>
	struct hash<PassengerID> {
		size_t operator()(const PassengerID& pid) const {
			return std::hash<PassengerID::IDType>()(pid.getValue());
		}
	};
}

namespace std {
	template<>
	struct hash<Passenger> {
		size_t operator()(const Passenger& p) const {
			return std::hash<PassengerID>()(p.getID());
		}
	};
}

#endif /* UTIL__PASSENGER_H */
