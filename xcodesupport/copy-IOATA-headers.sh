#!/bin/sh

#  copy-IOATA-headers.sh
#  PlatformKEXTs
#
#  Created by William Kent on 11/25/16.
#  Copyright © 2016 William Kent. All rights reserved.

set -e
PUBLIC_DIR="${XNU_INSTALL_ROOT}/System/Library/Frameworks/Kernel.framework/Versions/Current/Headers/IOKit/ata"
PRIVATE_DIR="${XNU_INSTALL_ROOT}/System/Library/Frameworks/Kernel.framework/Versions/Current/PrivateHeaders/IOKit/ata"

mkdir -p "${PUBLIC_DIR}" "${PRIVATE_DIR}"
cp "${SRCROOT}/IOATAFamily/IOATAFamilyPriv.h" "${PRIVATE_DIR}/IOATAFamilyPriv.h"

for FILE in IOPCIATA.h IOATABusCommand.h IOATABusInfo.h IOATACommand.h IOATAController.h IOATADevConfig.h IOATADevice.h IOATATypes.h ATADeviceNub.h ATATimerEventSource.h IOATARegI186.h; do
	cp "${SRCROOT}/IOATAFamily/${FILE}" "${PUBLIC_DIR}/${FILE}"
done
