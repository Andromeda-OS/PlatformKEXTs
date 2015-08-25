/*
 * Copyright (c) 2003-2007 Apple Inc. All Rights Reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#include <libkern/OSKextLib.h>

asm(".constructors_used = 0");
asm(".private_extern .constructors_used");

extern kmod_start_func_t *_realmain;
extern kmod_info_t KMOD_INFO_NAME;

// Functions defined in libkern/c++/OSRuntime.cpp
extern kern_return_t OSRuntimeInitializeCPP(kmod_info_t *ki, void *data);
extern kern_return_t OSRuntimeFinalizeCPP(kmod_info_t *ki, void *data);

__attribute__((visibility("hidden"))) kern_return_t _start(kmod_info_t *ki, void *data) {
    kern_return_t result = OSRuntimeInitializeCPP(ki, data);

    if (result == KERN_SUCCESS && _realmain) {
        result = (*_realmain)(ki, data);

        // If _realmain failed, tear down C++.
        if (result != KERN_SUCCESS) OSRuntimeFinalizeCPP(ki, data);
    }

    return result;
}

__attribute__((visibility("hidden"))) const char *OSKextGetCurrentIdentifier(void) {
    return KMOD_INFO_NAME.name;
}

__attribute__((visibility("hidden"))) const char *OSKextGetCurrentVersionString(void) {
    return KMOD_INFO_NAME.version;
}

__attribute__((visibility("hidden"))) OSKextLoadTag OSKextGetCurrentLoadTag(void) {
    return (OSKextLoadTag) KMOD_INFO_NAME.id;
}
