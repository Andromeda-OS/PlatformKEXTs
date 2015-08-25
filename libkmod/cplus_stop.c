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

asm(".destructors_used = 0");
asm(".private_extern .destructors_used");

// This function is defined in libkern/c++/OSRuntime.cpp
extern kern_return_t OSRuntimeFinalizeCPP(kmod_info_t *ki, void *data);
extern kmod_stop_func_t *_antimain;

__attribute__((visibility("hidden"))) kern_return_t _stop(kmod_info_t *ki, void *data) {
    kern_return_t result = KERN_SUCCESS;

    if (_antimain) result = (*_antimain)(ki, data);
    if (result == KERN_SUCCESS) result = OSRuntimeFinalizeCPP(ki, data);

    return result;
}
