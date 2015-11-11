
#ifndef UTIL__PASSENGER_H
#define UTIL__PASSENGER_H

#include <util/track_network.h++>
#include <util/utils.h++>

#include <iosfwd>

struct PassengerIdTag { const static uint DEFAULT_VALUE = -1; };
using PassengerId = ::util::ID<uint,PassengerIdTag>;

class Passenger {
private:
	std::string name;
	PassengerId id;
	TrackNetwork::ID entry_id;
	TrackNetwork::ID exit_id;
	uint start_time;

public:
	Passenger(
		const std::string& name,
		PassengerId id,
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

	PassengerId getId() const { return id; }
	const std::string& getName() const { return name; }
	TrackNetwork::ID getEntryId() const { return entry_id; }
	TrackNetwork::ID getExitId() const { return exit_id; }
	uint getStartTime() const { return start_time; }

	bool operator==(const Passenger& rhs) const {
		return id == rhs.id;
	}
};

using PassengerIdList = std::vector<PassengerId>;
using PassengerList = std::vector<Passenger>;

// std::ostream& operator<<(std::ostresam& os, const Passenger& p);
std::ostream& operator<<(std::ostream& os, std::pair<const Passenger&,const TrackNetwork&> pair);

namespace std {
	template<>
	struct hash<PassengerId> {
		size_t operator()(const PassengerId& pid) const {
			return std::hash<PassengerId::IdType>()(pid.getValue());
		}
	};
}

namespace std {
	template<>
	struct hash<Passenger> {
		size_t operator()(const Passenger& p) const {
			return std::hash<PassengerId>()(p.getId());
		}
	};
}

#endif /* UTIL__PASSENGER_H */
