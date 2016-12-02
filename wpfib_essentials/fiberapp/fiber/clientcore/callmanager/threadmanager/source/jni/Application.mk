#NDK_TOOLCHAIN_VERSION=4.4.3
APP_PROJECT_PATH := $(shell pwd)
APP_BUILD_SCRIPT := $(APP_PROJECT_PATH)/Android.mk
APP_STL := gnustl_static
APP_OPTIM := release
APP_ABI := armeabi-v7a
APP_MODULES  := libbjntm
