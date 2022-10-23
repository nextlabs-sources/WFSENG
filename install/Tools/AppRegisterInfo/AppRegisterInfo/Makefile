# AppRegisterInfo
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

ifeq ($(NLEXTERNALGITDIR),)
  $(error NLEXTERNALGITDIR undefined.)
endif


#
# Variables
#


SOLUTION=../AppRegisterInfo.sln
PROJECT=AppRegisterInfo.csproj
TARGETS_SP=compile

# Release parameter
CPUTYPE=AnyCPU

# any CPU the target we set it as x64
ifneq ($(TARGETENVARCH),x86)
	TARGETENVARCH=x64
endif

ifneq ($(BUILDTYPE), debug)
	BUILDTYPE=release
endif

BIN_DIR=$(BUILDTYPE)_win_$(TARGETENVARCH)
BUILD_LOGFILE=$(BUILDTYPE)_build.log

# Makefile.vs contains target all which depends on $(TARGETS_SP)
include $(NLBUILDROOT)/build/Makefile.vs


$(info --------------------------------------------------------------------------)
$(info [Project])
$(info PROJECT=$(PROJECT))
$(info [Parameters])
$(info BUILDTYPE=$(BUILDTYPE))
$(info NLBUILDROOT=$(NLBUILDROOT))
$(info NLEXTERNALGITDIR=$(NLEXTERNALGITDIR))
$(info BUILDOUTPUTDIR=$(BUILDOUTPUTDIR))
$(info BIN_DIR=$(BIN_DIR))
$(info ---------------------------------------------------------------------------)

.PHONY: compile
compile:
	@echo ""
	@echo "Building $(PROJECT) ($(BUILDTYPE)) for AppRegisterInfo, output $(BUILDOUTPUTDIR)"
	rm -f $(BUILD_LOGFILE)
	$(MSVSIDE) $(PROJECT) /build "$(BUILDTYPE)|$(CPUTYPE)" /out "$(BIN_DIR)/$(BUILD_LOGFILE)" ; \
	COMPILE_STATUS=$$? ; \
	if [ -f $(BIN_DIR)/$(BUILD_LOGFILE) ] ; then \
		echo "[[DUMP BEGIN - $(BIN_DIR)/$(BUILD_LOGFILE)]]" ; \
		cat $(BIN_DIR)/$(BUILD_LOGFILE) ; \
		echo "[[DUMP END - $(BIN_DIR)/$(BUILD_LOGFILE)]]" ; \
	else \
		echo "WARNING: Cannot find $(BIN_DIR)/$(BUILD_LOGFILE)" ; \
	fi ; \
	exit $$COMPILE_STATUS

	cp -pf $(BIN_DIR)/AppRegisterInfo.exe $(NLBUILDROOT)/tools

.PHONY: clean
clean:
	@if [ -e ./Makefile.inc ]; then \
		rm -rf $(BIN_DIR)  \
			obj* *.suo *.ncb *.pdb $(BIN_DIR)/$(BUILD_LOGFILE) \
			$(NLBUILDROOT)/tools/AppRegisterInfo.exe  \
	fi
