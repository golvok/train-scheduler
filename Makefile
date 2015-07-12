.SUFFIXES:
.PHONY: all clean

# remove ALL implicit rules
.SUFFIXES:
MAKEFLAGS+=" -r"

BUILD_MODE ?= release

EXE_DIR=EXE/
OBJ_DIR=OBJ/
DEPS_DIR=DEPS/

# CXX = clang++ -stdlib=libstdc++
# CXX = g++

# put warning flags here - go to compiler *and* linker.
WARNING_FLAGS += -Wall -Wextra -pedantic -Weffc++ -Werror

# put other flags for both the compiler & linker here
EXTRA_FLAGS = -std=c++11 

# add flags for debugging
ifeq ($(BUILD_MODE),debug)
	EXTRA_FLAGS += -ggdb -D DEBUG
endif

LIBRARY_LINK_FLAGS += \
	-lpthread

INCLUDE_FLAGS += \
	-I .

CXXFLAGS += $(EXTRA_FLAGS) $(WARNING_FLAGS) $(INCLUDE_FLAGS)
LDFLAGS  += $(EXTRA_FLAGS) $(WARNING_FLAGS) $(LIBRARY_LINK_FLAGS)

# keep .o files
.PRECIOUS: $(OBJ_DIR)%.o

# define source directories
SOURCE_DIRS = algo/ util/ parsing/ ./

# compute all directories that might need creation
DIRS=$(EXE_DIR) $(OBJ_DIR) $(DEPS_DIR) \
	$(addprefix $(OBJ_DIR),$(SOURCE_DIRS)) \
	$(addprefix $(DEPS_DIR),$(SOURCE_DIRS))

# define executables
EXES=$(EXE_DIR)train-sch

all: $(EXES)

# add more dependencies here:
$(EXE_DIR)train-sch: \
	$(OBJ_DIR)algo/scheduler.o \
	$(OBJ_DIR)parsing/input_parser.o \
	$(OBJ_DIR)util/track_network.o \
	$(OBJ_DIR)util/utils.o \
	$(OBJ_DIR)main.o

# include all the dependency files, if any exist
EXISTING_DEP_FILES = \
	$(foreach dir,$(SOURCE_DIRS), \
		$(wildcard $(DEPS_DIR)$(dir)*.d) \
	)
ifneq (,$(EXISTING_DEP_FILES))
include $(EXISTING_DEP_FILES)
endif

# compile .c++ to .o
# second CC line generates the initial dependency file
# first sed line adds $(OBJ_DIR) prefixes to the dependency file,
# second one adds stub rules for each depended on file (make might
# complain with generated d
.SECONDEXPANSION:
$(OBJ_DIR)%.o: %.c++ | $(OBJ_DIR)$$(dir %) $(DEPS_DIR)$$(dir %)
	$(CXX) -c  $< -o  $@ $(CXXFLAGS)
	@$(CXX) -MM $< -MF $(DEPS_DIR)$<.d.tmp $(CXXFLAGS)
	@sed -e 's|.*:|$@:|' < $(DEPS_DIR)$<.d.tmp > $(DEPS_DIR)$<.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(DEPS_DIR)$<.d.tmp | fmt -1 | \
	 sed -e 's/^ *//' -e 's/$$/:/' >> $(DEPS_DIR)$<.d
	@rm -f $(DEPS_DIR)$<.d.tmp

# compile *.o's into an executable
$(EXE_DIR)%: | $(EXE_DIR)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(DIRS):
	mkdir -p $@

clean:
	-rm -f $(EXES);
	-if [ -a $(EXE_DIR)  ]; then rmdir --ignore-fail-on-non-empty $(EXE_DIR);  fi;

	for subdir in $(SOURCE_DIRS); do \
		if [ -a $(DEPS_DIR)$${subdir} ]; then \
			deps_subdir=$$(readlink --canonicalize $(DEPS_DIR)$${subdir})/; \
			echo $${deps_subdir}; \
			rm -f $${deps_subdir}*.d; \
			rmdir --ignore-fail-on-non-empty $${deps_subdir}; \
		fi; \
		if [ -a $(OBJ_DIR)$${subdir} ]; then \
			objs_subdir=$$(readlink --canonicalize $(OBJ_DIR)$${subdir})/; \
			echo $${objs_subdir}; \
			rm -f $${objs_subdir}*.o; \
			rmdir --ignore-fail-on-non-empty $${objs_subdir}; \
		fi; \
	done
