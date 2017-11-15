# This Makefile is very generic, and should not have to be modified much except
# for special cases.
#
# Build tools.

# These will get redefined in a cross-compiling environment.
#
CC = gcc
CXX = g++

# These are used when building tools needed for the build.
#
BUILD_CC = gcc
BUILD_CXX = g++

# Compiler and linker flags used during the build
#
OPT_CFLAGS = -O2
CFLAGS = $(OPT_CFLAGS)
PROG_CFLAGS = -fPIC

# Destinations.  DESTDIR is used when the build is used for packaging or
# cross-compilation.
#
DESTDIR ?=
prefix ?= /usr/local
bindir ?= $(prefix)/bin
sbindir ?= ${prefix}/sbin
datadir ?= $(prefix)/share
sysconfdir ?= /etc

PACKAGE = auto_label
DEFINES = 
INCLUDES = 

TARGETS = watch_hosts

OBJS_watch_hosts = watch_hosts.o daemonize.o
LIBS_watch_hosts =

OBJECTS = $(OBJS_watch_hosts)

BUILD_TARGETS =

OBJS_hello =

BUILD_OBJECTS =

REBUILDABLES =  $(OBJECTS) $(TARGETS) $(BUILD_OBJECTS) $(BUILD_TARGETS)

INSTALL_TARGETS = altab $(TARGETS)

all : $(BUILD_TARGETS) $(TARGETS)

install : $(INSTALL_TARGETS)
	install -d -m 0755 $(DESTDIR)$(sysconfdir)/$(PACKAGE)
	install -d -m 0755 $(DESTDIR)$(sbindir)
	install -m 0644 altab $(DESTDIR)$(sysconfdir)/$(PACKAGE)/altab
	install -m 0755 remountd $(DESTDIR)$(sbindir)/remountd
	install -m 0755 auto_label $(DESTDIR)$(sbindir)/auto_label

clean :
	rm -f $(REBUILDABLES)
	@echo Clean done

# Link using c++.  It handles both c and c++ objects.

$(TARGETS) : $(OBJECTS)
	$(CXX) $(LDFLAGS_$@) -o $@ $(OBJS_$@) $(LIBS_$@)

$(BUILD_TARGETS) : $(BUILD_OBJECTS)
	$(BUILD_CXX) $(LDFLAGS_$@) -o $@ $(OBJS_$@) $(LIBS_$@)

%.o : %.c
	$(CC) $(CFLAGS) $(DEFINES) -g $(INCLUDES) -o $@ -c $<

%.o : %.cc
	$(CXX) $(DEFINES) -g $(INCLUDES) -o $@ -c $<

%.bo : %.c
	$(BUILD_CC) $(DEFINES) -g $(INCLUDES) -o $@ -c $<

%.bo : %.cc
	$(BUILD_CXX) $(DEFINES) -g $(INCLUDES) -o $@ -c $<

# Add dependencies and special rules here


