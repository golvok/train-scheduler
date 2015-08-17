#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <util/passenger.h++>
#include <util/track_network.h++>

namespace algo {

class Schedule {
	std::string name;
	TrackNetwork::ID entry_id;
	TrackNetwork::ID exit_id;
	uint start_time;

public:
	Schedule(
		const std::string name,
		TrackNetwork::ID entry_id,
		TrackNetwork::ID exit_id,
		uint start_time
	)
		: name(name)
		, entry_id(entry_id)
		, exit_id(exit_id)
		, start_time(start_time)
	{}

	const std::string& getName() const { return name; }
	TrackNetwork::ID getEntryId() const { return entry_id; }
	TrackNetwork::ID getExitId() const { return exit_id; }
	uint getStartTime() const { return start_time; }

	bool operator==(const Schedule& rhs) const {
		return name == rhs.name;
	}
};

class Train {
	std::string name;
	TrackNetwork::ID entry_id;
	TrackNetwork::ID exit_id;
	uint deprture_time;
	uint arrival_time;
	uint speed;

public:
	Train(
		const std::string name,
		TrackNetwork::ID entry_id,
		TrackNetwork::ID exit_id,
		uint deprture_time,
		uint arrival_time,
		uint speed
	)
		: name(name)
		, entry_id(entry_id)
		, exit_id(exit_id)
		,deprture_time(deprture_time)
		,arrival_time(arrival_time)
		,speed(speed)
	{}

	const std::string& getName() const { return name; }
	TrackNetwork::ID getEntryId() const { return entry_id; }
	TrackNetwork::ID getExitId() const { return exit_id; }
	uint getDeprtureTime() const { return deprture_time; }
	uint getArrivalTime() const { return arrival_time; }
	uint getSpeed() const { return speed; }

	bool operator==(const Train& rhs) const {
		return name == rhs.name;
	}
};



int schedule(TrackNetwork& network, std::vector<Passenger>& passengers);

} // end namespace algo



#endif /* SCHEDULER_H */