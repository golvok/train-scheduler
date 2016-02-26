
#ifndef UTIL__PASSENGER_H
#define UTIL__PASSENGER_H

#include <util/track_network.h++>
#include <util/utils.h++>

#include <functional>
#include <iosfwd>

struct PassengerIdTag { const static uint DEFAULT_VALUE = -1; };
using PassengerId = ::util::ID<uint,PassengerIdTag>;

class Passenger {
private:
	std::string name;
	PassengerId id;
	TrackNetwork::ID entry_id;
	TrackNetwork::ID exit_id;
	TrackNetwork::Time start_time;

public:
	Passenger(
		const std::string& name,
		PassengerId id,
		TrackNetwork::ID entry_id,
		TrackNetwork::ID exit_id,
		TrackNetwork::Time start_time
	)
		: name(name)
		, id(id)
		, entry_id(entry_id)
		, exit_id(exit_id)
		, start_time(start_time)
	{}

	PassengerId getID() const { return id; }
	const std::string& getName() const { return name; }
	TrackNetwork::ID getEntryID() const { return entry_id; }
	TrackNetwork::ID getExitID() const { return exit_id; }
	TrackNetwork::Time getStartTime() const { return start_time; }

	bool operator==(const Passenger& rhs) const {
		return id == rhs.id;
	}
};

using PassengerIDList = std::vector<PassengerId>;
using PassengerList = std::vector<Passenger>;
using PassengerConstRefList = std::vector<std::reference_wrapper<const Passenger>>;

inline void passengerRefListAdd(PassengerConstRefList& list, const Passenger& p) {
	list.push_back(p);
}

inline void passengerRefListRemove(PassengerConstRefList& list, const Passenger& p) {
	list.erase(std::find_if(list.begin(), list.end(), [&](auto& test_p) {
		return test_p.get() == p;
	}));
}

inline auto passengerRefListInserter(PassengerConstRefList& list) {
	return std::back_inserter(list);
}

std::ostream& operator<<(std::ostream& os, const Passenger& p);
std::ostream& operator<<(std::ostream& os, const std::tuple<const Passenger&,const TrackNetwork&>& pair);

namespace std {
	template<>
	struct hash<PassengerId> {
		size_t operator()(const PassengerId& pid) const {
			return std::hash<PassengerId::IDType>()(pid.getValue());
		}
	};
}

namespace std {
	template<>
	struct hash<Passenger> {
		size_t operator()(const Passenger& p) const {
			return std::hash<PassengerId>()(p.getID());
		}
	};
}

#endif /* UTIL__PASSENGER_H */
