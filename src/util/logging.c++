
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
			PR_D1, PR_D2, PR_D3, PR_D4,
			SIM_D1, SIM_D2, SIM_D3,
			DATA_READ1,
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
		{ PR_D4, PR_D3, PR_D2, PR_D1, },
		{ SIM_D3, SIM_D2, SIM_D1, },
		{ DATA_READ1, },
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
		{ PR_D4, "PR_D4" },

		{ SIM_D1, "SIM_D1" },
		{ SIM_D2, "SIM_D2" },
		{ SIM_D3, "SIM_D3" },
		{ DATA_READ1, "DATA_READ1" },
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

LevelStream::~LevelStream() {
	// when the caller is done, dump the ss to src;
	flush();
}

void LevelStream::flush() {
	if (enabled()) {
		src->print(underlying_ss);
	}
	underlying_ss.clear();
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
