
SUBDIRS = src

all :

all clean :
	@for dir in $(SUBDIRS); do \
		$(MAKE) $@ -C $${dir}; \
	done
