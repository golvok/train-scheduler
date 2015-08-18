#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <util/passenger.h++>
#include <util/track_network.h++>

namespace algo {

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

		// getters
		const std::string& getName() const { return name; }
		TrackNetwork::ID getEntryId() const { return entry_id; }
		TrackNetwork::ID getExitId() const { return exit_id; }
		uint getDeprtureTime() const { return deprture_time; }
		uint getArrivalTime() const { return arrival_time; }
		uint getSpeed() const { return speed; }

		// setters
		void setDeprtureTime(uint d) { deprture_time = d; }
		void setArrivalTime(uint a) { arrival_time = a; }
		void setSpeed(uint s) { speed = s; }

		bool operator==(const Train& rhs) const {
			return name == rhs.name;
		}
};

class Schedule {
	std::string name;
	std::vector<Train> trains;

	public:
		Schedule(
			const std::string name
		)
			: name(name)
			,trains()
		{}

		// getters
		const std::string& getName() const { return name; }
		std::vector<Train> getTrains() const { return trains; }

		// setters

		bool operator==(const Schedule& rhs) const {
			return name == rhs.name;
		}
};

int schedule(TrackNetwork& network, std::vector<Passenger>& passengers);

} // end namespace algo



#endif /* SCHEDULER_H */