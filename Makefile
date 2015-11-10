.SUFFIXES:
.PHONY: all clean build_info

# remove ALL implicit rules
.SUFFIXES:
MAKEFLAGS+=" -r "

# add parallelism equal to number of cores
MAKEFLAGS+=" -j$(grep -c "vendor_id" /proc/cpuinfo) "

BUILD_MODE ?= debug

EXE_DIR=EXE/
OBJ_DIR=OBJ/
DEPS_DIR=DEPS/

# CXX = clang++ -stdlib=libstdc++
# CXX = g++

# put warning flags here - go to compiler *and* linker.
WARNING_FLAGS += -Wall -Wextra -pedantic -Weffc++ -Werror

# put other flags for both the compiler & linker here
EXTRA_FLAGS = -std=c++14

# add flags for debugging
ifeq ($(BUILD_MODE),debug)
	EXTRA_FLAGS += -ggdb -D DEBUG
endif

LIBRARY_LINK_FLAGS += \
	$(shell pkg-config --libs gtkmm-3.0) \
	-lpthread

INCLUDE_FLAGS += \
	-I .

GRAPHICS_INCL_FLAGS += $(shell pkg-config --cflags gtkmm-3.0)

CXXFLAGS += $(EXTRA_FLAGS) $(WARNING_FLAGS) $(INCLUDE_FLAGS)
LDFLAGS  += $(EXTRA_FLAGS) $(WARNING_FLAGS) $(LIBRARY_LINK_FLAGS)

# keep .o files
.PRECIOUS: $(OBJ_DIR)%.o

# define source directories
SOURCE_DIRS = algo/ graphics/ parsing/ util/ stats/ ./

# compute all directories that might need creation
DIRS=$(EXE_DIR) $(OBJ_DIR) $(DEPS_DIR) \
	$(addprefix $(OBJ_DIR),$(SOURCE_DIRS)) \
	$(addprefix $(DEPS_DIR),$(SOURCE_DIRS))

# define executables
EXES=$(EXE_DIR)train-sch

all: $(EXES) | build_info

build_info:
	@echo "Building with makeflags ${MAKEFLAGS}"
	@echo "In build mode ${BUILD_MODE}"

# add more dependencies here:
$(EXE_DIR)train-sch: \
	$(OBJ_DIR)algo/scheduler.o \
	$(OBJ_DIR)algo/passenger_routing.o \
	$(OBJ_DIR)algo/schedule_to_graph_adapter.o \
	$(OBJ_DIR)graphics/graphics.o \
	$(OBJ_DIR)graphics/trains_area.o \
	$(OBJ_DIR)graphics/trains_area_data.o \
	$(OBJ_DIR)graphics/utils.o \
	$(OBJ_DIR)parsing/input_parser.o \
	$(OBJ_DIR)parsing/cmdargs_parser.o \
	$(OBJ_DIR)util/logging.o \
	$(OBJ_DIR)util/passenger.o \
	$(OBJ_DIR)util/track_network.o \
	$(OBJ_DIR)util/thread_utils.o \
	$(OBJ_DIR)stats/report_engine.o \
	$(OBJ_DIR)main.o

# define extra flags for particular object files
# adds graphics include flags to everything in graphics dir
$(OBJ_DIR)graphics/%.o: INCLUDE_FLAGS+=$(GRAPHICS_INCL_FLAGS)

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
# complain with generated files
.SECONDEXPANSION:
$(OBJ_DIR)%.o: %.c++ | build_info $(OBJ_DIR)$$(dir %) $(DEPS_DIR)$$(dir %)
	$(CXX) -c  $(shell readlink --canonicalize $<) -o  $@ $(CXXFLAGS)
	@$(CXX) -MM $< -MF $(DEPS_DIR)$<.d.tmp $(CXXFLAGS)
	@sed -e 's|.*:|$@:|' < $(DEPS_DIR)$<.d.tmp > $(DEPS_DIR)$<.d
	@sed -e 's/.*://' -e 's/\\$$//' < $(DEPS_DIR)$<.d.tmp | fmt -1 | \
	 sed -e 's/^ *//' -e 's/$$/:/' >> $(DEPS_DIR)$<.d
	@rm -f $(DEPS_DIR)$<.d.tmp

# compile *.o's into an executable
$(EXE_DIR)%: | build_info $(EXE_DIR)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(DIRS):
	mkdir -p $@

clean:
	-rm -f $(EXES);
	-if [ -e $(EXE_DIR)  ]; then rmdir --ignore-fail-on-non-empty $(EXE_DIR);  fi;

	for subdir in $(SOURCE_DIRS); do \
		if [ -e $(DEPS_DIR)$${subdir} ]; then \
			deps_subdir=$$(readlink --canonicalize $(DEPS_DIR)$${subdir})/; \
			echo $${deps_subdir}; \
			rm -f $${deps_subdir}*.d; \
			rmdir --ignore-fail-on-non-empty $${deps_subdir}; \
		fi; \
		if [ -e $(OBJ_DIR)$${subdir} ]; then \
			objs_subdir=$$(readlink --canonicalize $(OBJ_DIR)$${subdir})/; \
			echo $${objs_subdir}; \
			rm -f $${objs_subdir}*.o; \
			rmdir --ignore-fail-on-non-empty $${objs_subdir}; \
		fi; \
	done
