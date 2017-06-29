
#include "passenger_generator.h++"

PassengerGeneratorFactory:PassengerGeneratorFactory(uint64_t seed)
	:seed(seed)
{ }

PassengerGenerator PassengerGeneratorFactory::sample() const {
	return {1};
}

PassengerGenerator::PassengerGenerator(uint64_t seed)
	:
{

}