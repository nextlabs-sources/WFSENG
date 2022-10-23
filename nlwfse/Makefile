# SDKWrapper
# Notes: To each Visual Studio compilation output. We chain a few lines of scripts
# together. In doing so, Make executes all of them in one process. We want the exit 
# status of this process to be the exit status of code compilation. So we add this 
# line of code at the end. Note that there is no line continuation character "\" at
# the end of this line because this is the end of a chain of commands to be executed
# in one shell process. However, the line before this should have a "\".
#		exit $$COMPILE_STATUS
#
# On the other hand, if we put MSVC call within a if statement where the whole compilation
# binary signing and copying to distribution directory are all in one line. That means
# the command "exit $$COMPILE_STATUS" cannot be the last commnand to be executed in a
# process. In this case, we need to check exit status before calling "exit $$COMPILE_STATUS".
#		if [ $$COMPILE_STATUS -ne 0 ]; then		\
#			exit $$COMPILE_STATUS ;				\
#		fi ;									\
# $(BUILDOUTPUTDIR) is $(NLBUILDROOT)/bin/$(BIN_DIR)
#

ifeq ($(NLBUILDROOT),)
  $(error NLBUILDROOT undefined.)
endif

ifeq ($(NLEXTERNALDIR),)
  $(error NLEXTERNALDIR undefined.)
endif

# Always include Makefile.inc, and first to take precedence
include $(NLBUILDROOT)/build/Makefile.ver
include Makefile.inc

#
# Variables
#

PROJECT_NAME=nlwfse
SOLUTION=$(NLBUILDROOT)/$(PROJECT_NAME).sln
PROJECT=$(PROJECT_NAME).vcxproj
TARGETS_SP=compile
RCSRC = $(NLBUILDROOT)/nlwfse/nlwfse.rc

# Release parameter
CPUTYPE=x64

# any CPU the target we set it as x64
TARGETENVARCH=x64

ifneq ($(BUILDTYPE), debug)
	BUILDTYPE=release
endif

BIN_DIR=$(TARGETENVARCH)
BUILD_LOGFILE=$(BUILDTYPE)_build.log

# Makefile.vs contains target all which depends on $(TARGETS_SP)
include $(NLBUILDROOT)/build/Makefile.vs


$(info --------------------------------------------------------------------------)
$(info [Project])
$(info PROJECT=$(PROJECT))
$(info [Parameters])
$(info BUILDTYPE=$(BUILDTYPE))
$(info NLBUILDROOT=$(NLBUILDROOT))
$(info NLEXTERNALDIR=$(NLEXTERNALDIR))
$(info BUILDOUTPUTDIR=$(BUILDOUTPUTDIR))
$(info BIN_DIR=$(BIN_DIR))
$(info ---------------------------------------------------------------------------)

.PHONY: compile
compile:
	@echo ""
	@echo "Building $(PROJECT) ($(BUILDTYPE)) for NextLabs Windows File Server Enforcer Driver, output $(BUILDOUTPUTDIR)"
	rm -rf $(BUILD_LOGFILE)
	$(MSVSIDE) $(SOLUTION) /build "$(BUILDTYPE)|$(CPUTYPE)" /out $(BUILD_LOGFILE) ; \
	COMPILE_STATUS=$$? ; \
	if [ -f $(BUILD_LOGFILE) ] ; then \
		echo "[[DUMP BEGIN - $(BUILD_LOGFILE)]]" ; \
		cat $(BUILD_LOGFILE) ; \
		echo "[[DUMP END - $(BUILD_LOGFILE)]]" ; \
	else \
		echo "WARNING: Cannot find $(BUILD_LOGFILE)" ; \
	fi ; \
	exit $$COMPILE_STATUS
