#!/bin/sh

set -e

DESTDIR="${XNU_INSTALL_ROOT}/${SYSTEM_LIBRARY_DIR}/Frameworks/Kernel.framework/Versions/A/Headers/IOKit/acpi"
mkdir -p "${DESTDIR}"

cp -f "${SRCROOT}/IOACPIFamily/IOACPIPlatformDevice.h" "${DESTDIR}"
cp -f "${SRCROOT}/IOACPIFamily/IOACPIPlatformExpert.h" "${DESTDIR}"
cp -f "${SRCROOT}/IOACPIFamily/IOACPITypes.h" "${DESTDIR}"
