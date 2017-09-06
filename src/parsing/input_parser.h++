#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H

#include <util/passenger.h++>
#include <util/track_network.h++>

#include <iosfwd>

namespace parsing {
namespace input {

using StatPassCollection = std::vector<StatisticalPassenger>;

/**
 * returns the input data for the program to work on
 */
std::tuple<TrackNetwork,StatPassCollection, bool> parse_data(std::istream& is);

} // end namespace parsing
} // end namespace input

#endif /* INPUT_PARSER_H */
