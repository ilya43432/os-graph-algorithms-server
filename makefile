STAGES := Part_1 Part_2 Part_3 Part_6 Part_7 Part_8 Part_9 

.PHONY: all clean $(STAGES)

all: $(STAGES)

$(STAGES):
	$(MAKE) -C $@

clean:
	@for dir in $(STAGES); do \
		$(MAKE) -C $$dir clean; \
	done