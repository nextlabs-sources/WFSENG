
VERSION_PRODUCT		= $(VERSION_PRODUCT_PC)
VERSION_MAJOR		= $(VERSION_MAJOR_PC)
VERSION_MINOR		= $(VERSION_MINOR_PC)
VERSION_MAINTENANCE = $(VERSION_MAINTENANCE_PC)
VERSION_PATCH		= $(VERSION_PATCH_PC)

COMPILER=VS2008
PLATFORMSDK=7.0

TARGET = sdk_loader_test

SRC = \
      src/sdk_loader_test.cpp

INCPATH += \
  . \
  $(NLENFORCERSDIR)/platforms/common/brain/include \
  $(NLENFORCERSDIR)/platforms/win32/modules/eframework/eframework.R2 \
  $(NLENFORCERSDIR)/sdk/include

LIBPATH += \
  $(NLENFORCERSDIR)/platforms/win32/modules/eframework/eframework.R2/eframework/sdk_loader/$(BINDIR)

CUSTOM_DEFINES =

CFLAGS   += $(CUSTOM_DEFINES)
CXXFLAGS += $(CUSTOM_DEFINES)
