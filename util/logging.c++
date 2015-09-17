
#include "logging.h++"


IndentingLeveledDebugPrinter dout(std::cout, 0);


namespace DebugLevel {
	std::vector<Level> getDefaultSet() {
		return {
			INFO,WARN,ERROR
		};
	}

	std::vector<Level> getAllDebug() {
		return {
			WC_D1, WC_D2, WC_D3,
			TR_D1, TR_D2, TR_D3,
			PR_D1, PR_D2, PR_D3,
		};
	}

	/**
	 * The enable chains.
	 * if a level is in one of these, then it and all the ones after it
	 * will be returned by getAllShouldBeEnabled(...).
	 * a given Level may appear in multiple chains.
	 */
	std::vector<std::vector<Level>> enable_chains {
		{ WC_D3, WC_D2, WC_D1, },
		{ TR_D3, TR_D2, TR_D1, },
		{ PR_D3, PR_D2, PR_D1, },
	};

	std::vector<Level> getAllShouldBeEnabled(Level l) {
		std::vector<Level> retset;
		for (auto& enable_chain : enable_chains) {
			// find the Level
			auto it = std::find(begin(enable_chain),end(enable_chain),l);
			// insert *it and the ones after
			// if it == .end() (ie. l was not found), then nothing happens!
			retset.insert(retset.end(),it,enable_chain.end());
		}
		return retset;
	}

	std::vector<std::pair<Level,std::string>> levels_and_strings {
		{ INFO,  "INFO"  },
		{ WARN,  "WARN"  },
		{ ERROR, "ERROR" },
		{ WC_D1, "WC_D1" },
		{ WC_D2, "WC_D2" },
		{ WC_D3, "WC_D3" },
		{ TR_D1, "TR_D1" },
		{ TR_D2, "TR_D2" },
		{ TR_D3, "TR_D3" },
		{ PR_D1, "PR_D1" },
		{ PR_D2, "PR_D2" },
		{ PR_D3, "PR_D3" },
	};

	std::pair<Level,bool> getFromString(std::string str) {
		// linear search is probably fine
		for (auto& pair : levels_and_strings) {
			if (str == pair.second) {
				return {pair.first,true};
			}
		}
		return {Level::LEVEL_COUNT,false};
	}

	std::string getAsString(Level l) {
		// linear search is probably fine
		for (auto& pair : levels_and_strings) {
			if (l == pair.first) {
				return pair.second;
			}
		}

		dout(DL::WARN) << "trying to get the name of a level "
			"that doesn't have an associated string ( value = " << l << " )\n"
		;

		return "";
	}

}

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
