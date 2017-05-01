#!/bin/sh

set -e
INPUT_DIR=${SRCROOT}/IOPCIFamily/Headers/pci

function InstallHeaders {
	local DIR=$1
	shift

	mkdir -p $DIR
	cp -f "$@" $DIR
}

InstallHeaders "${XNU_INSTALL_ROOT}/System/Library/Frameworks/Kernel.framework/Versions/A/Headers/IOKit/pci" \
	"${INPUT_DIR}/IOAGPDevice.h" "${INPUT_DIR}/IOPCIBridge.h" "${INPUT_DIR}/IOPCIDevice.h"
InstallHeaders "${XNU_INSTALL_ROOT}/System/Library/Frameworks/Kernel.framework/Versions/A/PrivateHeaders/IOKit/pci" \
	"${INPUT_DIR}/IOPCIConfigurator.h" "${INPUT_DIR}/IOPCIPrivate.h"
InstallHeaders "${XNU_INSTALL_ROOT}/System/Library/Frameworks/IOKit.framework/Versions/A/PrivateHeaders/pci" \
	"${INPUT_DIR}/IOPCIDevice.h" "${INPUT_DIR}/IOPCIPrivate.h"
