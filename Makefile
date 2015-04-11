# Change these:
FINAL_BIN := ttymidi

WARNING_FLAGS := -Wall -Wcast-align -Wcast-align -Wextra -Wfloat-equal -Winit-self -Winline -Wmissing-declarations -Wmissing-include-dirs -Wno-long-long -Wpointer-arith -Wredundant-decls -Wredundant-decls -Wshadow -Wswitch-default -Wswitch-enum -Wundef -Wuninitialized -Wunreachable-code -Wwrite-strings

OPTIMIZATION_FLAGS := -march=corei7 -fexpensive-optimizations -Os

CFLAGS  := $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) $(STD_FLAG)

LD_FLAGS  := -pthread

STD_FLAG  :=

#CC_FLAGS  := $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) $(STD_FLAG)



#-------------------------------------------------------------------------------
# These probably don't need to be changed (but can be)
SRC_DIR := src
DEP_DIR := .build/dep
OBJ_DIR := .build/obj
BIN_DIR := .

CC       := gcc



#-------------------------------------------------------------------------------
# Things that don't really need to be changed:
C_FILES   := $(wildcard $(SRC_DIR)/*.c)
H_FILES   := $(wildcard $(SRC_DIR)/*.h)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.c.o,$(C_FILES))
DEP_FILES := $(patsubst $(OBJ_DIR)/%.c.o, $(DEP_DIR)/%.c.o.d, $(OBJ_FILES))


.PHONY: all clean cleanall

all: $(BIN_DIR)/$(FINAL_BIN)

cleanall: clean

clean:
	rm -f $(OBJ_FILES) $(BIN_DIR)/$(FINAL_BIN) $(DEP_FILES)

# Compile the final binary
$(BIN_DIR)/$(FINAL_BIN): $(OBJ_FILES)
	@ echo "gcc -o $@ $^"
	@ $(CC) $(LD_FLAGS) $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) -o $@ $^

# Compile each of the object files
$(OBJ_DIR)/%.c.o: $(DEP_DIR)/%.c.o.d
	@ echo "gcc -c -o $@ $<"
	@ $(CC) $(WARNING_FLAGS) $(OPTIMIZATION_FLAGS) $(STD_FLAG) -c -o $@ $(SRC_DIR)/$*.c

# Create dependency file
$(DEP_DIR)/%.c.o.d: $(SRC_DIR)/%.c
	@ echo "Creating deps for $^"
	@ $(CPP) $(STD_FLAG) $< -MM -MF $@ -MT $(patsubst $(DEP_DIR)/%.c.o.d, $(OBJ_DIR)/%.c.o, $@)



#-------------------------------------------------------------------------------
# Include the dependency files

# "-" = suppressed errors for when there are no files to include
-include $(DEP_FILES)
