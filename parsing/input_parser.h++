#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H

#include <util/track_network.h++>
#include <util/utils.h++>

#include <iosfwd>

namespace parsing {
namespace input {

std::tuple<TrackNetwork,std::vector<Train>, bool> parse_graph(std::istream& is, std::ostream& err);

} // end namespace parsing
} // end namespace input

#endif /* INPUT_PARSER_H */
