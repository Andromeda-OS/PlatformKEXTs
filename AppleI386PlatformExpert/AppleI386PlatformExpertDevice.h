/*
 * Portions Copyright (c) 1999-2003 Apple Computer, Inc. All Rights
 * Reserved.
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * This file was modified by William Kent in 2015 to support the Andromeda
 * project. This notice is included in support of clause 2.2(b) of the License.
 */

#ifndef _IOKIT_APPLEI386PLATFORMEXPERTDEVICE_H
#define _IOKIT_APPLEI386PLATFORMEXPERTDEVICE_H

#include <IOKit/IOPlatformExpert.h>

class AppleI386PlatformExpertDevice : IOPlatformExpertDevice {
    OSDeclareDefaultStructors(AppleI386PlatformExpertDevice);

public:
    virtual IOService * probe(IOService *provider, SInt32 *score);
};

#endif
