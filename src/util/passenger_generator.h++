
#ifndef UTIL__PASSENGER_GENERATOR_H
#define UTIL__PASSENGER_GENERATOR_H

#include <util/generator.h++>
#include <util/passenger.h++>

#include <random>

#include <boost/operators.hpp>

class PassengerGenerator;
class StationPassengerGenerator;

class PassengerGeneratorFactory {
public:
	using PassengerGeneratorCollection = std::vector<PassengerGenerator>;
	using Seed = std::uint_fast64_t;

	PassengerGeneratorFactory(Seed seed, const std::vector<StatisticalPassenger>& statpsgrs)
		: seed(seed)
		, statpsgrs(statpsgrs)
	{ }

	PassengerGeneratorCollection sample() const;
private:
	Seed seed;
	const std::vector<StatisticalPassenger>& statpsgrs;
};

class PassengerGenerator {
public:
	using Seed = PassengerGeneratorFactory::Seed;
	using RandGen = std::mt19937_64;
	using Time = TrackNetwork::Time;

	template<typename TIME, typename PID_GENERATOR>
	auto leavingDuringInterval(TIME begin_time, TIME end_time, PID_GENERATOR& pid_generator) const {
		struct State : boost::equality_comparable<State> {
			const PassengerGenerator* src;
			Time next_departure;
			TIME end_time;

			State(const PassengerGenerator* src, TrackNetwork::Time next_departure, TIME end_time)
				: src(src)
				, next_departure(next_departure)
				, end_time(end_time)
			{ }

			bool operator==(const State& rhs) const {
				return std::tie(src, next_departure)
					== std::tie(rhs.src, rhs.next_departure);
			}
		};

		return util::make_generator<State>(
			// initial
			State(this, nextPassengerAfter(begin_time), end_time),
			// done
			[](const State& s) {
				return s.next_departure >= s.end_time;
			},
			// next
			[](State&& s) {
				s.next_departure = s.src->nextPassengerAfter(s.next_departure);
				return s;
			},
			// transform
			[&](const State& s) {
				return instantiateAt(&s.src->statpsgr, pid_generator.gen_id(), s.next_departure);
			}
		);
	}

private:
	friend PassengerGeneratorFactory;
	PassengerGenerator(const StatisticalPassenger& statpsgr, Seed seed);

	Time nextPassengerAfter(TrackNetwork::Time t) const;

	const StatisticalPassenger statpsgr;
	const Seed seed;
	RandGen rand_gen;
	// mutable std::unordered_set<???> interval_cache;
};

#endif /* UTIL__PASSENGER_GENERATOR_H */
