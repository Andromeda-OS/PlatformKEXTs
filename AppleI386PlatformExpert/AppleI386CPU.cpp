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

#include "AppleI386CPU.h"

#undef super
#define super IOCPU

static void _creating_AppleI386CPU(void) {
    kprintf("Loading AppleI386CPU\n");
}

OSDefineMetaClassAndStructorsWithInit(AppleI386CPU, IOCPU, _creating_AppleI386CPU());

IOService *AppleI386CPU::probe(IOService *provider, SInt32 *score) {
    kprintf("Inside AppleI386CPU::probe()\n");
    if (score) *score = 10000;
    return this;
}

bool AppleI386CPU::startCommon() {
    if (startCommonCompleted) return true;
    
    cpuIC = new AppleI386CPUInterruptController;
    if (cpuIC == 0) return false;
    kprintf("AppleI386CPUInterruptController::initCPUInterruptController()\n");
    if (cpuIC->initCPUInterruptController(1) != kIOReturnSuccess) return false;
    kprintf("AppleI386CPUInterruptController::initCPUInterruptController() - success\n");
    
    cpuIC->attach(this);
    cpuIC->registerCPUInterruptController();
    
    setCPUState(kIOCPUStateUninitalized);
    initCPU(true);
    registerService();
    
    startCommonCompleted = true;
    return true;
}

bool AppleI386CPU::start(IOService *provider) {
    kprintf("Inside AppleI386CPU::start()\n");
    if (!super::start(provider)) return false;

    return startCommon();
}

void AppleI386CPU::initCPU(bool boot) {
    cpuIC->enableCPUInterrupt(this);
    setCPUState(kIOCPUStateRunning);
}

void AppleI386CPU::quiesceCPU() {
    // Not required.
}

kern_return_t AppleI386CPU::startCPU(vm_offset_t start_paddr, vm_offset_t arg_paddr) {
    // Not implemented.
    return KERN_FAILURE;
}

void AppleI386CPU::haltCPU() {
    // Not required.
}

const OSSymbol *AppleI386CPU::getCPUName() {
    return OSSymbol::withCStringNoCopy("Primary0");
}

#pragma mark -
#undef super
#define super IOCPUInterruptController

OSDefineMetaClassAndStructors(AppleI386CPUInterruptController, IOCPUInterruptController);

IOReturn AppleI386CPUInterruptController::handleInterrupt(void *refCon, IOService *nub, int source) {
    // Override the implementation in IOCPUInterruptController to
    // dispatch interrupts the old way. The source argument is ignored;
    // the first IOCPUInterruptController in the vector array is always used.

    IOInterruptVector *vector = &vectors[0];
    if (!vector->interruptRegistered) return kIOReturnInvalid;

    vector->handler(vector->target, refCon, vector->nub, source);
    return kIOReturnSuccess;
}
