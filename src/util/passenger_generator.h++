
#ifndef UTIL__PASSENGER_GENERATOR_H
#define UTIL__PASSENGER_GENERATOR_H

#include <util/generator.h++>

class PassengerGenerator;
class StationPassengerGenerator;

class PassengerGenerationFactory {
public:
	PassengerGenerationFactory(uint64_t seed);

	PassengerGenerator sample() const;
private:
	uint64_t seed;
};

class PassengerGenerator {
public:
	StationPassengerGenerator forStation(StationID sid) const;

private:
	friend class PassengerGenerationFactory;
	PassengerGenerator(uint64_t seed);

	uint64_t seed;
}

class StationPassengerGenerator {
public:
	template<typename TIME>
	auto leavingDuringInterval(TIME begin_time, TIME end_time) {
		struct State {
			TIME next;
			int random;
		};

		return util::make_generator(
			// initial
			state{begin_time, 1},
			// done
			[&](const State& s) {
				return s.next >= end_time;
			},
			// next
			[&](State&& s) {
				s.next += s.random;
				return s;
			},
			// transform
			[&](const State& s) {
				return s.next;
			}
		);
	}

private:
	friend class PassengerGenerator;
	StationPassengerGenerator(const PassengerGenerator& src)
		: src(src)
	{ }

	const PassengerGenerator& src;
}

#endif /* UTIL__PASSENGER_GENERATOR_H */

/*
 * likely input will be station -> {(dest with frequency info)+}
 * consumer: what leaves in this interval (and from where)
*/
