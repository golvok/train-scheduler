
#include "passenger_generator.h++"

auto PassengerGeneratorFactory::sample() const -> PassengerGeneratorCollection {
	(void)seed;
	PassengerGeneratorCollection result;
	std::transform(begin(statpsgrs), end(statpsgrs), std::back_inserter(result), [&](const auto& statpsgr) {
		return PassengerGenerator(statpsgr);
	});
	return result;
}

TrackNetwork::Time PassengerGenerator::nextPassengerAfter(TrackNetwork::Time t) const {
	const auto next = 5 * std::ceil(t/5.0);
	if (next == t) {
		return next + 5;
	} else {
		return next;
	}
}
