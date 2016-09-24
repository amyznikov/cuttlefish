# Devilfish makefile

SHELL=/bin/bash
.ONESHELL:

ARCH = native

############################################################

include arch/$(ARCH)/config
include arch/$(ARCH)/toolchain


# order is important due to dependency issues
ALL_MODULES = \
	cuttlessl \
	protobuf \
	cuttle \
	ffmpeg \
	x264

export AS
export CC
export CPP
export CXX
export LD
export LDXX
#export AR
export NM
export STRIP
export OBJCOPY
export OBJDUMP
export HOST_CC
export HOST_CXX
export HOST_LD
export HOST_LDXX

############################################################

if-enabled = \
	$(if $(or $(filter y,$(with-$1)),$(filter s,$(with-$1))),$2,)

if-build = \
	$(if $(filter y,$(with-$1)),$2,$3)

build-filter = \
	$(foreach m, $1, $(if $(filter y,$(with-$m)),$m,))


uniq = \
	$(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))



############################################################

BUILD_MODULES = $(call build-filter, $(ALL_MODULES))
OBJDIR = $(CURDIR)/arch/$(ARCH)/build
INSTALLDIR=$(CURDIR)/arch/$(ARCH)/special-is
PREFIX ?= /usr/local
CPPFLAGS += -I$(INSTALLDIR)/$(PREFIX)/include
LDFLAGS += -L$(INSTALLDIR)/$(PREFIX)/lib -L$(INSTALLDIR)/$(PREFIX)/lib64
LDXXFLAGS += -L$(INSTALLDIR)/$(PREFIX)/lib -L$(INSTALLDIR)/$(PREFIX)/lib64

ifeq ($(ARCH),native)
	PATH:=$(INSTALLDIR)/$(PREFIX)/bin:$(PATH)
	export PATH
	LD_LIBRARY_PATH:=$(INSTALLDIR)/$(PREFIX)/lib:$(INSTALLDIR)/$(PREFIX)/lib64:$(LD_LIBRARY_PATH)
	export LD_LIBRARY_PATH
else
	cross_host = $(ARCH)
	cross_prefix = $(ARCH)-
	enable_cross_compile = --enable-cross-compile --arch=$(ARCH)
	GRPC_CROSS_COMPILE=true
endif



all: $(BUILD_MODULES)


CLEAN_ALL = $(addprefix clean-,$(BUILD_MODULES))
clean: $(CLEAN_ALL)
$(CLEAN_ALL) :
	@if [ -d $(OBJDIR)/$(@:clean-%=%) ] ; then $(MAKE) -i -C $(OBJDIR)/$(@:clean-%=%) clean; fi
	@if [ -d modules/$(@:clean-%=%)   ] ; then $(MAKE) -i -C modules/$(@:clean-%=%) clean ; fi


DISTCLEAN_ALL = $(addprefix distclean-,$(BUILD_MODULES))
distclean: $(DISTCLEAN_ALL)
$(DISTCLEAN_ALL) :
	rm -rf $(OBJDIR)/$(@:distclean-%=%)
	$(MAKE) -i -C modules/$(@:distclean-%=%) clean distclean || true


PACKAGE_ALL = $(addprefix package-,$(BUILD_MODULES))
package: $(PACKAGE_ALL)
$(PACKAGE_ALL):
	cd distrib/$(@:package-%=%) && \
	$(CURDIR)/buildpkg \
		ARCH=$(ARCH) \
		NDK_ROOT=$(NDK_ROOT) \
		NDK_PLATFORM=$(NDK_PLATFORM) \
		PREFIX=$(PREFIX) \
		SOURCE_DIR=$(CURDIR)/modules/$(@:package-%=%) \
		GRPC_CONFIG=$(GRPC_CONFIG) \
		${pkgargs}
	

$(OBJDIR)/% :
	mkdir -p $@

$(INSTALLDIR):
	mkdir -p $@



############################################################

cuttlessl: configure-cuttlessl build-cuttlessl install-cuttlessl

install-cuttlessl: $(INSTALLDIR)
	$(MAKE) -C $(CURDIR)/modules/cuttlessl install_sw
	mkdir -p $(INSTALLDIR)/$(PREFIX)/include/openssl/engines/ccgost && cp $(CURDIR)/modules/cuttlessl/engines/ccgost/*.h $(INSTALLDIR)/$(PREFIX)/include/openssl/engines/ccgost 

uninstall-cuttlessl:
	rm -f $(INSTALLDIR)/$(PREFIX)/bin/{c_rehash,openssl}
	rm -rf $(INSTALLDIR)/$(PREFIX)/include/openssl $(INSTALLDIR)/$(PREFIX)/ssl
	rm -f $(INSTALLDIR)/$(PREFIX)/lib/libcrypto* $(INSTALLDIR)/$(PREFIX)/lib/libssl* $(INSTALLDIR)/$(PREFIX)/lib/pkgconfig/{libcrypto.pc,libssl.pc,openssl.pc}
	rm -rf $(INSTALLDIR)/$(PREFIX)/lib/engines 


build-cuttlessl: # fixme? makedepend: command not found
	$(MAKE) -C $(CURDIR)/modules/cuttlessl all

configure-cuttlessl:
	if [[ "$(ARCH)" == "native" ]] ; then \
		cd $(CURDIR)/modules/cuttlessl && ./config --prefix="$(PREFIX)" --install_prefix="$(INSTALLDIR)" shared || exit 1 ; \
	elif [[ "$(ARCH)" == "arm-linux-androideabi"* ]] ; then \
		cd $(CURDIR)/modules/cuttlessl && \
			./Configure --prefix="$(PREFIX)"  --install_prefix=$(INSTALLDIR) android-armv7 || exit 1 ; \
	else \
		cd $(CURDIR)/modules/cuttlessl && \
			./Configure --prefix="$(PREFIX)" --cross-compile-prefix=$(cross_prefix) --install_prefix=$(INSTALLDIR) FIXME || exit 1 ; \
	fi


############################################################

protobuf: configure-protobuf build-protobuf install-protobuf

configure-protobuf: $(CURDIR)/modules/protobuf/config.h

build-protobuf:
	$(MAKE) -C $(CURDIR)/modules/protobuf V=1 all

install-protobuf: $(INSTALLDIR)
	$(MAKE) -C $(CURDIR)/modules/protobuf V=1 install DESTDIR=$(INSTALLDIR)

uninstall-protobuf:
	$(MAKE) -C $(CURDIR)/modules/protobuf V=1 uninstall DESTDIR=$(INSTALLDIR)
	rm -rf $(INSTALLDIR)/$(PREFIX)/include/google/protobuf

$(CURDIR)/modules/protobuf/config.h: $(CURDIR)/modules/protobuf/configure
	cd $(CURDIR)/modules/protobuf && ./configure \
		--prefix=$(PREFIX) \
		--host=$(cross_host) \
		--enable-static=yes \
		--enable-shared=no \
		--enable-dependency-tracking=yes \
		--enable-fast-install=no \
		--with-pic=yes

#--enable-shared still does not work correctly with DESTDIR - not relinking corretly ptotoc
#	sed -i 's:libfile=$$libdir:libfile=$$DESTDIR/$$libdir:' libtool


$(CURDIR)/modules/protobuf/configure: $(CURDIR)/modules/protobuf/autogen.sh
	cd $(CURDIR)/modules/protobuf && ./autogen.sh

############################################################

cuttle: build-cuttle install-cuttle

BUILD_CUTTLE_ARGS = \
	ARCH=$(ARCH) \
	prefix=$(PREFIX) \
	NDK_PLATFORM=$(NDK_PLATFORM) \
	NDK_ROOT=$(NDK_ROOT) \
	CC="$(CC)" \

build-cuttle:
	CPPFLAGS="$(CPPFLAGS)" \
		$(MAKE) -C $(CURDIR)/modules/cuttle \
			$(BUILD_CUTTLE_ARGS) \
			all

configure-cuttle:
	@echo "Skip $@: nothing to do"

install-cuttle: $(INSTALLDIR)
	CPPFLAGS="$(CPPFLAGS)" \
		$(MAKE) -C $(CURDIR)/modules/cuttle \
			$(BUILD_CUTTLE_ARGS) \
			DESTDIR=$(INSTALLDIR) \
			install

uninstall-cuttle:
	CPPFLAGS="$(CPPFLAGS)" \
		$(MAKE) -C $(CURDIR)/modules/libcuttle \
			$(BUILD_CUTTLE_ARGS) \
			DESTDIR=$(INSTALLDIR) \
			uninstall


############################################################


ffmpeg: configure-ffmpeg build-ffmpeg install-ffmpeg

configure-ffmpeg: $(OBJDIR)/ffmpeg $(call build-filter, openssl x264)
	cd $(OBJDIR)/ffmpeg && \
	$(CURDIR)/modules/ffmpeg/configure \
		$(enable_cross_compile) \
		--PREFIX=$(PREFIX) \
		--target-os=linux \
		--disable-doc \
		--disable-ffserver \
		--enable-avresample \
		--enable-gpl \
		--enable-version3 \
		--enable-nonfree \
		$(call if-enabled,openssl,--enable-openssl) \
		$(call if-enabled,x264,--enable-libx264) \
		$(call if-enabled,lame,--enable-libmp3lame) \
		$(call if-enabled,opencore,--enable-libopencore-amrnb --enable-libopencore-amrwb) \
		--cc=$(CC) \
		--cxx=$(CXX) \
		--ar=$(AR) \
		--as=$(CC) \
		--strip=$(STRIP) \
		--extra-cflags="$(CPPFLAGS)" \
		--extra-cxxflags="$(CPPFLAGS)" \
		--extra-ldflags="$(LDFLAGS)" \
		--extra-ldexeflags="$(LDFLAGS)"

build-ffmpeg:
	$(MAKE) -C $(OBJDIR)/ffmpeg V=1 all

install-ffmpeg: $(INSTALLDIR)
	$(MAKE) -C $(OBJDIR)/ffmpeg V=1 install DESTDIR=$(INSTALLDIR)




############################################################

x264: configure-x264 build-x264 install-x264

configure-x264: $(CURDIR)/modules/x264/config.h

build-x264:
	 $(MAKE) -C $(CURDIR)/modules/x264 V=1

install-x264 : $(INSTALLDIR)
	$(MAKE) -C $(CURDIR)/modules/x264 V=1 install-cli install-lib-static install-lib-shared DESTDIR=$(INSTALLDIR)

uninstall-x264 :
	$(MAKE) -C $(CURDIR)/modules/x264 V=1 uninstall DESTDIR=$(INSTALLDIR)


$(CURDIR)/modules/x264/config.h:
	@echo -e "\n--------\n$@"
	cd $(CURDIR)/modules/x264 && ./configure \
			--PREFIX=$(PREFIX) \
			--cross-PREFIX=$(cross_prefix) \
			--host=$(cross_host) \
			--enable-static \
			--chroma-format=all \
			--enable-shared \
			--disable-opencl \
			--disable-cli \
			--extra-cflags="$(CPPFLAGS)" \
			--extra-ldflags="$(LDFLAGS)"


############################################################
