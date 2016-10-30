#!/bin/sh

KERN_DESTDIR="${XNU_INSTALL_ROOT}${SYSTEM_LIBRARY_DIR}/Frameworks/Kernel.framework/Versions/A/Headers/IOKit/storage"
USER_DESTDIR="${XNU_INSTALL_ROOT}${SYSTEM_LIBRARY_DIR}/Frameworks/IOKit.framework/Versions/A/Headers/storage"
mkdir -p "${USER_DESTDIR}" "${KERN_DESTDIR}"

for FILE in $(ls ${SRCROOT}/IOStorageFamily/*.h); do
	cp -f $FILE $KERN_DESTDIR

	BASE=$(basename $FILE)
	unifdef -UKERNEL "${FILE}" > "${USER_DESTDIR}/${BASE}"
done
