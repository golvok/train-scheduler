#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <util/passenger.h++>
#include <util/track_network.h++>

namespace algo {

class Train {
	uint train_id;
	TrackNetwork::ID entry_id;
	TrackNetwork::ID exit_id;
	uint deprture_time;
	uint arrival_time;
	uint speed;

	public:
		Train(
			const uint train_id,
			TrackNetwork::ID entry_id,
			TrackNetwork::ID exit_id,
			uint deprture_time,
			uint arrival_time,
			uint speed
		)
			: train_id(train_id)
			, entry_id(entry_id)
			, exit_id(exit_id)
			,deprture_time(deprture_time)
			,arrival_time(arrival_time)
			,speed(speed)
		{}

		// getters
		uint gettrain_id() const { return train_id; }
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
			return train_id == rhs.train_id;
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

		void addTrain();
		void removeTrain();
		void clear();
};

/**
 * Main entry point into the scheduler
 *
 * Call this function to do scheduling
 */
Schedule schedule(
	const TrackNetwork& network,
	const std::vector<Passenger>& passengers
);

} // end namespace algo



#endif /* SCHEDULER_H */