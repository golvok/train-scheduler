
#include "passenger_generator.h++"

auto PassengerGeneratorFactory::sample() const -> PassengerGeneratorCollection {
	(void)seed;
	PassengerGeneratorCollection result;
	PassengerGenerator::Seed s = seed;
	std::transform(begin(statpsgrs), end(statpsgrs), std::back_inserter(result), [&](const auto& statpsgr) {
		s += 1;
		return PassengerGenerator(statpsgr, s);
	});
	return result;
}

PassengerGenerator::PassengerGenerator(const StatisticalPassenger& statpsgr, Seed seed)
	: statpsgr(statpsgr)
	, seed(seed)
	, rand_gen(seed)
{ }

auto PassengerGenerator::nextPassengerAfter(Time t) const -> Time {
	RandGen rand_gen_l(seed);
	std::geometric_distribution<Time> geom_dist(statpsgr.getAverageRate());

	Time prev = 0;

	while (true) {
		if (prev > t) {
			return prev;
		}
		const auto num = geom_dist(rand_gen_l);
		prev += num;
	}
}
