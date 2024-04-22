/* pci.h
   Purpose: Support for PCI devices */
#pragma once

#include "abstract/interrupt_ctl_interface.h"
#include "arch/i386/idt.h"
#include "config.h"

#define PCI_DONT_CARE -1

#define PCI_ERROR_OK 0
#define PCI_ERROR_NOT_SUPPORTED 1

struct pci_search_query 
{
    uint16_t Vendor, DeviceId;
    uint8_t Class, SubClass;

    /* Not really used for SearchDevice, 
       but we need for LoadReason */
    uint8_t Bus, Function, Device;
};

struct pci_device
{
    struct pci_device* Next;
    uint8_t Bus, Function, Device;
};

struct pci_device;
struct pci_device* pciSearchDevice(uint16_t vendor, uint16_t device);
struct pci_device* pciSearchDeviceEx(struct pci_search_query);
uint32_t pciReadDoubleWord(uint8_t bus, uint8_t device, 
                           uint8_t function, uint8_t offset);
uint32_t pciReadDoubleWordFromDevice(const struct pci_device* device, 
                                     uint8_t offset);
void pciWriteDoubleWordToDevice(const struct pci_device*,
                                uint8_t offset, uint32_t data);
int pciHandleMessageInterrupt(struct pci_device* dev, 
                               pfn_idt_interrupt_t pfn, void* data);

void pciInitializeRegistry();