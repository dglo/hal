$(BUILD_DIR)/$(PUB_DIR_NAME)/%.h : $(PLATFORM)/%.awk
	@test -d $(@D) || mkdir -p $(@D)
	$(GAWK)  -f $(<D)/common.awk -f $< < $(<D)/$(CONFIG) > $@

