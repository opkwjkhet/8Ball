ARCHS = arm64
DEBUG = 0
FINALPACKAGE = 1
FOR_RELEASE = 1
IGNORE_WARNINGS = 0
TARGET = iphone:clang:latest:14.0
THEOS_PACKAGE_SCHEME = rootless

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = 8Ball

$(TWEAK_NAME)_FRAMEWORKS = UIKit Foundation Metal MetalKit CoreFoundation CoreGraphics QuartzCore CoreText IOKit

$(TWEAK_NAME)_CFLAGS = -fobjc-arc -Wall -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-value -Wno-unused-function -fno-unwind-tables -fno-asynchronous-unwind-tables
$(TWEAK_NAME)_CCFLAGS = -std=c++17 -fno-rtti -fno-exceptions -DNDEBUG -Wall -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-value -Wno-unused-function -fvisibility=hidden -Wno-unused-private-field -Wno-tautological-undefined-compare -fexceptions
$(TWEAK_NAME)_LDFLAGS = -Wl,-w
$(TWEAK_NAME)_FILES = ImGuiDrawView.mm $(wildcard LoadView/*.m) $(wildcard ImGui/*.cpp) $(wildcard ImGui/*.mm) $(wildcard autoclick/*.m)

include $(THEOS_MAKE_PATH)/tweak.mk

.PHONY: all clean package install run

run: package install
