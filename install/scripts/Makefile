ifeq ($(NLBUILDROOT),)
  $(error NLBUILDROOT undefined.)
endif

ifeq ($(NLEXTERNALDIR),)
  $(error NLEXTERNALDIR undefined.)
endif

include $(NLBUILDROOT)/build/Makefile.ver

PRODUCT_DIR_NAME=wfse
VERSION_STR=$(VERSION_MAJOR_WFSE).$(VERSION_MINOR_WFSE)
ISM_TEMPLATE_FILE_NAME=WindowsFileServerEnforcer_x64.ism
ISM_RELEASE=msi
ISM_PRODUCT_CONFIG64=64bit
MSI_FILE64=CE-WindowsFileServerEnforcer64.$(ISM_RELEASE)
TARGETMSI_FILE64_NAME=CE-WindowsFileServerEnforcer-setup64.$(ISM_RELEASE)

ifeq ($(BUILDTYPE),)
	BUILDTYPE=dev
endif

ifeq ($(VERSION_BUILD),)
	VERSION_BUILD=1
endif

include $(NLBUILDROOT)/install/scripts/Makefile.install
