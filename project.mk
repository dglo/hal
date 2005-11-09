C_BIN_NAMES :=
USES_PROJECTS := dom-loader dom-fpga dom-cpld
USES_TOOLS :=

ifneq ("epxa10","$(strip $(PLATFORM))")
  USES_PROJECTS := $(filter-out dom-cpld, $(USES_PROJECTS))
endif
