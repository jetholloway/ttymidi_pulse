# Change these:
FINAL_BIN := ttymidi_pulse

WARNING_FLAGS := -Wall -Wextra -Wshadow -Wcast-align -Wconversion -Wextra -Wfloat-equal -Winit-self -Winline -Wmissing-declarations -Wmissing-include-dirs -Wno-long-long -Wpointer-arith -Wredundant-decls -Wshadow -Wswitch-default -Wswitch-enum -Wundef -Wuninitialized -Wunreachable-code -Wwrite-strings

OPTIMIZATION_FLAGS := -march=corei7 -fexpensive-optimizations -Os

LD_FLAGS  :=  -ldbus-1 -lglib-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 -pthread

CPP_STD_FLAG  := -std=c++11

C_STD_FLAG  :=

CPP_FLAGS  := $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) $(CPP_STD_FLAG) -g3 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/

CC_FLAGS  := $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) $(STD_FLAG)



#-------------------------------------------------------------------------------
# These probably don't need to be changed (but can be)
SRC_DIR := src
DEP_DIR := .build/dep
OBJ_DIR := .build/obj
BIN_DIR := .

CPP       := g++
CC       := gcc



#-------------------------------------------------------------------------------
# Things that don't really need to be changed:
CPP_FILES := $(wildcard $(SRC_DIR)/*.cpp)
C_FILES   := $(wildcard $(SRC_DIR)/*.c)
CPP_OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.cpp.o,$(CPP_FILES))
C_OBJ_FILES   := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.c.o,$(C_FILES))
DEP_FILES     := $(patsubst $(OBJ_DIR)/%.o, $(DEP_DIR)/%.o.d, $(OBJ_FILES))

.PHONY: all clean cleanall debug

all: $(BIN_DIR)/$(FINAL_BIN)

cleanall: clean

clean:
	rm -f $(CPP_OBJ_FILES) $(C_OBJ_FILES) $(BIN_DIR)/$(FINAL_BIN) $(DEP_FILES)

debug: CPP_FLAGS += -DDEBUG -g
debug: CC_FLAGS += -DDEBUG -g
debug: $(BIN_DIR)/$(FINAL_BIN)

# Compile the final binary
$(BIN_DIR)/$(FINAL_BIN): $(CPP_OBJ_FILES) $(C_OBJ_FILES)
	@ echo "gcc -o $@ $^"
	@ $(CPP) $(LD_FLAGS) $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) -o $@ $^

# Compile each of the CPP object files
$(OBJ_DIR)/%.cpp.o: $(SRC_DIR)/%.cpp $(DEP_DIR)/%.cpp.o.d
	@ echo "c++ -c -o $@ $<"
	@ $(CPP) $(CPP_FLAGS) -c -o $@ $<

# Compile each of the C object files
$(OBJ_DIR)/%.c.o: $(DEP_DIR)/%.c.o.d
	@ echo "gcc -c -o $@ $<"
	@ $(CC) $(CC_FLAGS) -c -o $@ $(SRC_DIR)/$*.c

# Create dependency file
$(DEP_DIR)/%.cpp.o.d: $(SRC_DIR)/%.cpp
	@ echo "Creating deps for $^"
	$(CPP) $(CPP_STD_FLAG) $< -MM -MF $@ -MT $(patsubst $(DEP_DIR)/%.cpp.o.d, $(OBJ_DIR)/%.cpp.o, $@)

# Create dependency file
$(DEP_DIR)/%.c.o.d: $(SRC_DIR)/%.c
	@ echo "Creating deps for $^"
	@ $(CPP) $(C_STD_FLAG) $< -MM -MF $@ -MT $(patsubst $(DEP_DIR)/%.c.o.d, $(OBJ_DIR)/%.c.o, $@)



#-------------------------------------------------------------------------------
# Include the dependency files

# "-" = suppressed errors for when there are no files to include
-include $(DEP_FILES)
