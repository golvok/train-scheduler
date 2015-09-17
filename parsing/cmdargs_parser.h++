
#ifndef PARSING__CMDARGS_PARSER_H
#define PARSING__CMDARGS_PARSER_H

namespace parsing {

namespace cmdargs {

class ParsedArguments {
public:
	bool shouldEnableGraphics() { return graphics_enabled; }

private:
	friend ParsedArguments parse(int arc_int, char const** argv);

	bool graphics_enabled;

	ParsedArguments(int arc_int, char const** argv);
};

ParsedArguments parse(int arc_int, char const** argv);

} // end namespace cmdargs

} // end namespace parsing

#endif /* PARSING__CMDARGS_PARSER_H */
