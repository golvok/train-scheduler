
#ifndef UTIL__PASSENGER_H
#define UTIL__PASSENGER_H

#include <util/track_network.h++>

#include <iosfwd>

class Passenger {
public:
	using ID = uint;
private:
	std::string name;
	ID id;
	TrackNetwork::ID entry_id;
	TrackNetwork::ID exit_id;
	uint start_time;

public:
	Passenger(
		const std::string& name,
		ID id,
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

	ID getId() const { return id; }
	const std::string& getName() const { return name; }
	TrackNetwork::ID getEntryId() const { return entry_id; }
	TrackNetwork::ID getExitId() const { return exit_id; }
	uint getStartTime() const { return start_time; }

	bool operator==(const Passenger& rhs) const {
		return id == rhs.id;
	}
};

// std::ostream& operator<<(std::ostresam& os, const Passenger& p);
std::ostream& operator<<(std::ostream& os, std::pair<const Passenger&,const TrackNetwork&> pair);

namespace std {
	template<>
	struct hash<Passenger> {
		size_t operator()(const Passenger& p) const {
			return std::hash<Passenger::ID>()(p.getId());
		}
	};
}

#endif /* UTIL__PASSENGER_H */
