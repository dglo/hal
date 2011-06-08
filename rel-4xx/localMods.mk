GAWK := gawk
CONFIG := config.dom-mb.micron

# Add auto-generated include files to list of includes
#   Build list be translating AWK file names into include file names.
AWK_FILES_SEARCH := $(patsubst %,%/*.awk, $(PUB_DIRS) $(PVT_DIRS))
AWK_IGNORE_FILES := $(patsubst %,%/common.awk, $(PUB_DIRS) $(PVT_DIRS))
AWK_FILES := $(filter-out $(AWK_IGNORE_FILES), $(wildcard $(AWK_FILES_SEARCH)))
AWK_TARGETS := $(subst $(PUB_DIR_NAME)/$(PLATFORM), $(PUB_DIR_NAME), $(AWK_FILES))
AWK_TARGETS := $(subst $(PVT_DIR_NAME)/$(PLATFORM), $(PVT_DIR_NAME), $(AWK_TARGETS))
BUILT_HDRS := $(patsubst %.awk, $(BUILD_DIR)/%.$(C_INC_SUFFIX), $(AWK_TARGETS))

BUILT_FILES := $(BUILT_HDRS)

vpath %.awk $(PUB_DIR_NAME)/$(PLATFORM) $(PUB_DIR_NAME) $(PVT_DIR_NAME)/$(PLATFORM) $(PVT_DIR_NAME)

LOAD_LIBS := $(filter-out -ldom-fpga, $(LOAD_LIBS))
