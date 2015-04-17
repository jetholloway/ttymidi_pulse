#!/usr/bin/make -f

# Change these:
FINAL_BIN := ttymidi_pulse

WARNING_FLAGS := -Wall -Wcast-align -Wconversion -Wextra -Wfloat-equal -Winit-self -Winline -Wmissing-declarations -Wmissing-include-dirs -Wno-long-long -Wpointer-arith -Wredundant-decls -Wshadow -Wswitch-default -Wswitch-enum -Wundef -Wuninitialized -Wunreachable-code -Wwrite-strings

OPTIMIZATION_FLAGS := -march=corei7 -fexpensive-optimizations -Os

LD_FLAGS :=
LD_LIBS  :=  -ldbus-1 -lglib-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 -pthread

# Passed only to C++ compiler
CPP_STD_FLAG  := -std=c++11

CPP_FLAGS  := $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) $(CPP_STD_FLAG) $(TARGET_CPP_FLAGS) -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/



#-------------------------------------------------------------------------------
# These probably don't need to be changed (but can be)
SRC_DIR := src
DEP_DIR := .build/dep
OBJ_DIR := .build/obj
BIN_DIR := .

CPP     := g++

# Conditional: only called on outer make process
ifndef TARGET_DIR
.PHONY: all release debug clean clean-release clean-debug

all: release

clean: clean-release clean-debug

# Define targets and variables here
release: export TARGET_DIR := release
release: export TARGET_SUFFIX :=
debug:   export TARGET_DIR := debug
debug:   export TARGET_SUFFIX := -dbg
debug:   export TARGET_CPP_FLAGS := -DDEBUG -g3

clean-release: export TARGET_DIR := release
clean-release: export TARGET_SUFFIX :=
clean-debug:   export TARGET_DIR := debug
clean-debug:   export TARGET_SUFFIX := -dbg

clean-release clean-debug:
	@$(MAKE) clean-target

# Call make subprocess with the target
release debug:
	@$(MAKE) final-bin-target
else



#-------------------------------------------------------------------------------
# Things that don't really need to be changed:
# Everthing in this section gets run in a make sub-process, with TARGET_DIR set
REAL_DEP_DIR := $(DEP_DIR)/$(TARGET_DIR)
REAL_OBJ_DIR := $(OBJ_DIR)/$(TARGET_DIR)
REAL_FINAL_BIN := $(FINAL_BIN)$(TARGET_SUFFIX)

CPP_FILES     := $(wildcard $(SRC_DIR)/*.cpp)
CPP_OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(REAL_OBJ_DIR)/%.cpp.o,$(CPP_FILES))
CPP_DEP_FILES := $(patsubst $(REAL_OBJ_DIR)/%.cpp.o, $(REAL_DEP_DIR)/%.cpp.o.d, $(CPP_OBJ_FILES))

.PHONY: clean-target final-bin-target

final-bin-target: $(BIN_DIR)/$(REAL_FINAL_BIN)

clean-target:
	rm -f $(CPP_OBJ_FILES) $(BIN_DIR)/$(REAL_FINAL_BIN) $(CPP_DEP_FILES)

# Compile the final binary
$(BIN_DIR)/$(REAL_FINAL_BIN): $(CPP_OBJ_FILES)
	@ echo "$(CPP) -o $@ $^"
	@ $(CPP) $(LD_FLAGS) $(WARNING_FLAGS) -o $@ $^ $(LD_LIBS)

# Compile each of the C++ object files
$(REAL_OBJ_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp $(REAL_DEP_DIR)/%.cpp.o.d
	@ echo "$(CPP) -c -o $@ $<"
	@ $(CPP) $(CPP_FLAGS) -c -o $@ $<

# Create dependency file
$(REAL_DEP_DIR)/%.cpp.o.d: $(SRC_DIR)/%.cpp
	$(CPP) $(CPP_STD_FLAG) $< -MM -MF $@ -MT $(REAL_OBJ_DIR)/$*.cpp.o



#-------------------------------------------------------------------------------
# Include the dependency files

# "-" = suppressed errors for when there are no files to include
-include $(CPP_DEP_FILES)

endif # TARGET_DIR
