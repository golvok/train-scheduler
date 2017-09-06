
#ifndef UTIL__PASSENGER_H
#define UTIL__PASSENGER_H

#include <util/print_printable.h++>
#include <util/track_network.h++>
#include <util/utils.h++>

#include <functional>
#include <iosfwd>

struct PassengerIDTag { const static uint DEFAULT_VALUE = -1; };
using PassengerID = ::util::ID<uint,PassengerIDTag>;

template<typename STREAM>
STREAM& operator<<(STREAM&& os, const PassengerID& pid) {
	os << "PID" << pid.getValue();
	return os;
}

class StatisticalPassenger
	: public util::print_printable
	, public util::print_with_printable<const TrackNetwork> {
public:
	StatisticalPassenger(
		const std::string& base_name,
		TrackNetwork::NodeID entry_id,
		TrackNetwork::NodeID exit_id
	)
		: base_name(base_name)
		, entry_id(entry_id)
		, exit_id(exit_id)
	{ }

	const std::string& getBaseName() const { return base_name; }
	TrackNetwork::NodeID getEntryID() const { return entry_id; }
	TrackNetwork::NodeID getExitID() const { return exit_id; }

	void print(std::ostream& os) const;
	void print(std::ostream& os, const TrackNetwork& tn) const;

private:
	std::string base_name;
	TrackNetwork::NodeID entry_id;
	TrackNetwork::NodeID exit_id;
};

std::ostream& operator<<(std::ostream& os, const StatisticalPassenger& p);

class Passenger
	: public util::print_printable
	, public util::print_with_printable<const TrackNetwork> {
public:
	Passenger(
		const StatisticalPassenger* src,
		PassengerID id,
		TrackNetwork::Time start_time
	)
		: src(src)
		, id(id)
		, start_time(start_time)
	{ }

	const std::string& getName() const { return src->getBaseName(); }
		// + tn.getVertexName(elem.entrance) + '_' + tn.getVertexName(elem.exit)
	TrackNetwork::NodeID getEntryID() const { return src->getEntryID(); }
	TrackNetwork::NodeID getExitID() const { return src->getExitID(); }
	TrackNetwork::Time getStartTime() const { return start_time; }
	PassengerID getID() const { return id; }

	operator PassengerID() const { return getID(); }

	bool operator==(const Passenger& rhs) const {
		return id == rhs.id;
	}

	void print(std::ostream& os) const;
	void print(std::ostream& os, const TrackNetwork& tn) const;

private:
	const StatisticalPassenger* src;
	PassengerID id;
	TrackNetwork::Time start_time;
};

inline Passenger instantiateAt(const StatisticalPassenger* sp, PassengerID id, TrackNetwork::Time t) {
	return Passenger(sp, id, t);
}

using PassengerIDList = std::vector<PassengerID>;
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
