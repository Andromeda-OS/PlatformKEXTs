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

#include <IOKit/IOLib.h>
#include <IOKit/assert.h>
#include <IOKit/system.h>
#include <IOKit/IORegistryEntry.h>
#include <IOKit/platform/ApplePlatformExpert.h>
#include <libkern/c++/OSContainers.h>
#include <libkern/c++/OSUnserialize.h>
#include <pexpert/i386/boot.h>

extern "C" {
#include <i386/cpuid.h>
}

#include "AppleI386PlatformExpert.h"

__BEGIN_DECLS
extern void kdreboot(void);
__END_DECLS

enum {
    kIRQAvailable   = 0,
    kIRQExclusive   = 1,
    kIRQSharable    = 2,
    kSystemIRQCount = 16
};

static struct {
    UInt16 consumers;
    UInt16 status;
} IRQ[kSystemIRQCount];

static IOLock *ResourceLock;

class AppleI386PlatformExpertGlobals {
public:
    bool isValid;
    AppleI386PlatformExpertGlobals();
    ~AppleI386PlatformExpertGlobals();
};

static AppleI386PlatformExpertGlobals AppleI386PlatformExpertGlobals;
AppleI386PlatformExpertGlobals::AppleI386PlatformExpertGlobals() {
    ResourceLock = IOLockAlloc();
    bzero(IRQ, sizeof(IRQ));
}

AppleI386PlatformExpertGlobals::~AppleI386PlatformExpertGlobals() {
    if (ResourceLock) IOLockFree(ResourceLock);
}

#pragma mark -

#define super IOPlatformExpert

OSDefineMetaClassAndStructors(AppleI386PlatformExpert, IOPlatformExpert);

IOService *AppleI386PlatformExpert::probe(IOService *provider, SInt32 *score) {
    return this;
}

bool AppleI386PlatformExpert::init(OSDictionary *properties) {
    if (!super::init()) return false;
    _interruptControllerName = OSSymbol::withString((OSString *)getProperty("InterruptControllerName"));
    return true;
}

bool AppleI386PlatformExpert::start(IOService *provider) {
    setBootROMType(kBootROMTypeNewWorld);

    if (!super::start(provider)) return false;
    PE_halt_restart = handlePEHaltRestart;
    registerService();

    return true;
}

bool AppleI386PlatformExpert::configure(IOService *provider) {
    OSArray *topLevel;
    OSDictionary *dict;
    IOService *nub;

    topLevel = OSDynamicCast(OSArray, getProperty("top-level"));

    if (topLevel) {
        while ((dict = OSDynamicCast(OSDictionary, topLevel->getObject(0)))) {
            dict->retain();
            topLevel->removeObject(0);
            nub = createNub(dict);
            if (nub == 0) continue;

            dict->release();
            nub->attach(this);
            nub->registerService();
        }
    }

    return true;
}

IOService *AppleI386PlatformExpert::createNub(OSDictionary *from) {
    IOService *nub;

    nub = super::createNub(from);
    if (nub) {
        const char *name = nub->getName();

        if (strcmp(name, "pci") == 0) {
            // TODO: Get the PCI info from the boot args
            // and set it as the `pci-bus-info` property in the `from` dict.
        } else if (strcmp(name, "bios") == 0) {
            setupBIOS(nub);
        } else if (strcmp(name, "8259-pic") == 0) {
            setupPIC(nub);
        }
    }

    return nub;
}

void AppleI386PlatformExpert::setupPIC(IOService *nub) {
    int i;
    OSDictionary *propTable;
    OSArray *controller;
    OSArray *specifier;
    OSData *tmpData;
    long tmpLong;

    propTable = nub->getPropertyTable();

    // For the moment... assume a classic 8259 interrupt controller
    // with 16 interrupts. Later, this will be changed to detect
    // an APIC and/or MP-Table and then will set the nubs appropriately.

    specifier = OSArray::withCapacity(kSystemIRQCount);
    assert(specifier);

    for (i = 0; i < kSystemIRQCount; i++) {
        tmpLong = i;
        tmpData = OSData::withBytes(&tmpLong, sizeof(tmpLong));
        specifier->setObject(tmpData);
    }

    controller = OSArray::withCapacity(kSystemIRQCount);
    assert(controller);

    for (i = 0; i < kSystemIRQCount; i++) controller->setObject(_interruptControllerName);

    propTable->setObject(gIOInterruptControllersKey, controller);
    propTable->setObject(gIOInterruptSpecifiersKey, specifier);

    specifier->release();
    controller->release();
}

void AppleI386PlatformExpert::setupBIOS(IOService *nub) {
    // TODO: Implement this function.
    // This function is dependent upon being able to retrieve the
    // PCI bus data. While the booter does collect some PCI data,
    // but it does not include the data needed here.
}
