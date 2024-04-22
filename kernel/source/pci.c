#include "pci.h"
#include "abstract/interrupt_ctl.h"
#include "arch/i386/idt.h"
#include "arch/i386/ports.h"
#include "config.h"
#include "memory/physical.h"
#include "term/terminal.h"

/* NOTICE: This PCI driver impl relies
   that the system is x86_64. Either move it to
   arch/i386 or make it more arch independent */

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA    0xCFC

struct pci_device* devices = NULL;

static void devicesAppend(struct pci_device** head, struct pci_device* dev) 
{
    if (!*head) {
        (*head) = dev;
        return;
    }

    struct pci_device* tmp = *head;
    while (tmp->Next)
        tmp = tmp->Next;
    tmp->Next = dev;
}

/* vvvvv ACTUAL PCI vvvvv */

static void checkBus(uint8_t bus);
#define pciCompareGeneric(X, y, t) ((X == (t)PCI_DONT_CARE) || (X == y))
#define pciCompare(X, y) pciCompareGeneric(X, y, uint16_t)
#define pciCompareuint8(X, y) pciCompareGeneric(X, y, uint8_t)

uint32_t pciReadDoubleWord(uint8_t bus, uint8_t device, 
                            uint8_t function, uint8_t offset)
{
    uint32_t address = (1 << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11)
                     | ((uint32_t)function << 8) | (offset & 0xFC);
    outl(CONFIG_ADDRESS, address);
    return inl(CONFIG_DATA);
}

uint32_t pciReadDoubleWordFromDevice(const struct pci_device* device, 
                                     uint8_t offset)
{
    return pciReadDoubleWord(device->Bus, device->Device, 
                            device->Function, offset);
}

void pciWriteDoubleWord(uint8_t bus, uint8_t device, 
                            uint8_t function, uint8_t offset, 
                            uint32_t data)
{
    uint32_t address = (1 << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11)
                     | ((uint32_t)function << 8) | (offset & 0xFC);
    outl(CONFIG_ADDRESS, address);
    outl(CONFIG_DATA, data);
}

void pciWriteDoubleWordToDevice(const struct pci_device* dev,
                                uint8_t offset, uint32_t data)
{
    return pciWriteDoubleWord(dev->Bus, dev->Device, 
                              dev->Function, offset, data);
}

static void checkFunction(uint8_t bus, uint8_t device, uint8_t function) 
{
    uint32_t cls = (pciReadDoubleWord(bus, device, function, 8) & 0xFFFF0000) >> 16;
    uint32_t vd = (pciReadDoubleWord(bus, device, function, 0) & 0xFFFF);

    if (vd != 0xFFFF) {
        struct pci_device* dev = mmAllocKernelObject(struct pci_device);
        dev->Device = device;
        dev->Bus = bus;
        dev->Function = function;
        dev->Next = NULL;
        devicesAppend(&devices, dev);
    }

    if ((cls & 0xFF00) == 0x6 && (cls & 0x00FF) == 0x4) {
        uint32_t b2 = pciReadDoubleWord(bus, device, function, 0x18) >> 8;
        checkBus(b2 & 0xFF);
    }
}

static void checkDevice(uint8_t bus, uint8_t device) 
{
    uint8_t function = 0;

    if ((pciReadDoubleWord(bus, device, function, 0) & 0xFFFF) == 0xFFFF) {
        return; /* Invalid... */
    }

    checkFunction(bus, device, function);
    uint32_t ht = (pciReadDoubleWord(bus, device, function, 0xC) >> 16) & 0xFF;
    if (ht & (1 << 7)) {
        for (function = 1; function < 8; function++) {
            checkFunction(bus, device, function);
        }
    }
}

void checkBus(uint8_t bus)
{
    for (int i = 0; i < 32; i++) {
        checkDevice(bus, i);
    }
}

void pciInitializeRegistry() 
{
    checkBus(0);
}

struct pci_device* pciSearchDevice(uint16_t vendor, uint16_t device)
{
    struct pci_device* dev = devices;
    uint32_t vddv;
    uint16_t vend;
    uint32_t dend;

    while(dev->Next != NULL) {
        vddv = pciReadDoubleWord(dev->Bus, dev->Device, dev->Function, 0);
        vend = vddv & 0xFFFF;
        dend = vddv >> 16;

        if (vend == vendor && dend == device) {
            return dev;
        }

        dev = dev->Next;
    }

    return NULL;
}

struct pci_device* pciSearchDeviceEx(struct pci_search_query query)
{
    struct pci_device* dev = devices;
    uint32_t vddv, clscl;
    uint16_t vend;
    uint32_t dend;
    uint8_t class, subclass;

    while(dev->Next != NULL) {
        vddv = pciReadDoubleWord(dev->Bus, dev->Device, dev->Function, 0);
        clscl = pciReadDoubleWord(dev->Bus, dev->Device, dev->Function, 8) >> 16;

        vend = vddv & 0xFFFF;
        dend = vddv >> 16;
        subclass = clscl & 0xFF;
        class = (clscl >> 8) & 0xFF;
        
        if (pciCompare(query.Vendor, vend) && pciCompare(query.DeviceId, dend)
         && pciCompareuint8(query.Class, class) && pciCompareuint8(query.SubClass, subclass)) {
            return dev;
        }

        dev = dev->Next;
    }

    return NULL;
}

int pciHandleMessageInterrupt(struct pci_device* dev, 
                              pfn_idt_interrupt_t pfn, void* intData)
{
    uint32_t w = pciReadDoubleWordFromDevice(dev, 0x4) >> 16;
    if (!(w & (1 << 4))) {
        return PCI_ERROR_NOT_SUPPORTED;
    }

    bool found = false;
    uint32_t t;
    uint32_t offset = pciReadDoubleWordFromDevice(dev, 0x34) & 0xFC;
    while (offset) {
        t = pciReadDoubleWordFromDevice(dev, offset);
        if ((t & 0x00FF) == 0x05) {
            found = true;
            break;
        }

        offset = t & 0xFC00;
    }

    if (!found)
        return PCI_ERROR_NOT_SUPPORTED;

    int vector = idtGetFreeVector(3);
    int addend = 0x8;
    uint64_t data;
    uint64_t address = intCtlGetMsiAddress(&data, vector);

    uint16_t msgctl = (t & 0xFFFF0000) >> 16;
    pciWriteDoubleWordToDevice(dev, offset + 0x4, address & 0xFFFFFFFF);
    if (msgctl & (1 << 7)) {
        addend = 0xC;
        pciWriteDoubleWordToDevice(dev, offset + 0x8, address >> 32);
    }

    t &= ~(0xFFFF << 16);
    msgctl |= 1; /* Enable MSI */
    
    pciWriteDoubleWordToDevice(dev, offset, t | (msgctl << 16));
    pciWriteDoubleWordToDevice(dev, offset + addend, data & 0xFFFF);
    idtHandleInterrupt(vector, pfn, intData);
    return PCI_ERROR_OK;
}
