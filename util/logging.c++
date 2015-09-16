
#include "logging.h++"

template<typename SINK>
bool indent_filter::put(SINK& dest, int c) {
	// if we see a newline, remember it, and output spaces next time.;
	if (c == '\n') {
		just_saw_newline = true;
	} else if (just_saw_newline) {
		util::repeat(src.getNumSpacesToIndent(),[&](){
			boost::iostreams::put(dest,' ');
		});
		just_saw_newline = false;
	}
	return boost::iostreams::put(dest, c);
}

void IndentLevel::endIndent() {
	ended = true;
	if (src) {
		src->endIndent();
	}
}

IndentLevel::~IndentLevel() {
	if (src && !ended) {
		src->endIndent();
	}
}

IndentingDebugPrinter dout(std::cout, 0);
