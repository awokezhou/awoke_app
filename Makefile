
AWOKE_PATH:=$(subst /awoke_app,/awoke_app , $(shell pwd))
AWOKE_PATH:=$(word 1, $(AWOKE_PATH))

MAKE_PART := lib

.PHONY: $(MAKE_PART)
all: $(MAKE_PART)

$(MAKE_PART):
	$(MAKE) -C $@

.PHONY: clean
clean:
	for dir in $(MAKE_PART); do \
		$(MAKE) -C $$dir clean; \
	done

include $(AWOKE_PATH)/make.common