
MAKE_PART := libutil librtmsg libevent libsecurity libpattern libvinparser libhttp
.PHONY: $(MAKE_PART)
all: $(MAKE_PART)

$(MAKE_PART):
	$(MAKE) -C $@


.PHONY: clean
clean:
	for dir in $(MAKE_PART); do \
		$(MAKE) -C $$dir clean; \
	done

