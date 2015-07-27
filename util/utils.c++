#include "utils.h++"

void IndentLevel::endIndent() {
	ended = true;
	src.endIndent();
}

IndentLevel::~IndentLevel() {
	if (!ended) {
		src.endIndent();
	}
}

IndentingDebugPrinter dout(std::cout, 0);
