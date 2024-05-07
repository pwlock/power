#include "partition.h"
#include "memory/physical.h"
#include "mod/driver/driver.h"
#include "term/terminal.h"
#include "utils/string.h"
#include "utils/vector.h"

static struct vector partitions;
static struct vector disks;

static bool uuidNull(const uuid_t* u)
{
    /* TODO: refactor this */
    return u->Raw[0] == 0
        && u->Raw[1] == 0
        && u->Raw[2] == 0
        && u->Raw[3] == 0
        && u->Raw[4] == 0
        && u->Raw[5] == 0
        && u->Raw[6] == 0
        && u->Raw[7] == 0; 
}

bool uuidCompare(union ioctl_uuid* lhs, union ioctl_uuid* rhs)
{
    return lhs->Data1 == rhs->Data1
        && lhs->Data2 == rhs->Data2
        && lhs->Data3 == rhs->Data3
        && lhs->Data4[0] == rhs->Data4[0]
        && lhs->Data4[1] == rhs->Data4[1]
        && lhs->Data5[0] == rhs->Data5[0]
        && lhs->Data5[1] == rhs->Data5[1]
        && lhs->Data5[2] == rhs->Data5[2]
        && lhs->Data5[3] == rhs->Data5[3]
        && lhs->Data5[4] == rhs->Data5[4]
        && lhs->Data5[5] == rhs->Data5[5];
}

/* convert a 512-byte LBA to a secsz-byte LBA  */
static uint64_t get512Lba(unsigned int secsz, uint64_t lba)
{
    return (512 * lba) / secsz;
}

static bool handleGptPartition(struct driver_disk_device_interface* interf,
                               uint8_t* restrict scratch, struct vector* part)
{
    size_t sector = interf->getSectorSize(interf);
    interf->readSector(interf, get512Lba(sector, 1), sector, scratch);

    int addend = sector > 512 ? 512 : 0;
    const struct gpt_header gh = *(struct gpt_header*)(scratch + addend);
    if (strncmp(gh.Signature, "EFI PART", 8)) {
        trmLogfn("handleGptPartition: Not a GPT disk");
        return false;
    }

    size_t currentSize = sector,
           currentIndex = 0;
    size_t currentLba = get512Lba(sector, 2);
    addend = sector > 512 ? 1024 : 0;

    const struct gpt_partition* pat = (struct gpt_partition*)(scratch + addend);
    interf->readSector(interf, currentLba, sector, scratch);
    while (currentIndex < gh.PartitionArrayCount) {
        if (!currentSize) {
            /* Read the next LBA
               if there is more 
               partitions to be read. */
            if ((currentLba - gh.PartitionArrayBegin > 31)) {
                break;
            }

            currentLba++;
            pat = (struct gpt_partition*)scratch;
            int status = interf->readSector(interf, currentLba, sector, scratch);
            if (status < 0)
                return false;

            currentSize = sector;
        }

        if (!uuidNull(&pat->Type)) {
            struct partition* p = mmAllocKernelObject(struct partition);
            memcpy(p->Identifier.Raw, pat->Uid.Raw, sizeof(uuid_t));
            p->BeginLba = pat->FirstLba;
            p->EndLba = pat->LastLba;
            vectorInsert(part, p);
        }

        currentSize -= gh.PartitionArrayEntrySize;
        currentIndex++;
        pat++;
    }

    return true;
}

struct vector 
partGetPartitions(struct driver_disk_device_interface* interf)
{
    struct vector parts = vectorCreate(4);
    size_t secsz = interf->getSectorSize(interf);
    uint8_t* scratch = mmAllocKernel(secsz);
    interf->readSector(interf, 0, secsz, scratch);

    struct mbr_bootstrp* bs = (struct mbr_bootstrp*)scratch;
    struct mbr_partition* fpart = &bs->Partitions[0];
    if (fpart->PartitionType == 0xEE) {
        if (!handleGptPartition(interf, scratch, &parts))
            return vectorCreate(0);
    }

    mmAlignedFree(scratch, secsz);
    return parts;
}

void partInit(struct driver_disk_interface* di)
{
    size_t max = di->getDeviceCount(di);
    trmLogfn("max=%i", max);
    size_t length = 0;
    struct driver_disk_device_interface* dev = NULL;
    struct pt_disk* disk = NULL;
    struct vector partitions;

    while (true) {
        if (length >= max)
            break;

        dev = di->getDevice(di, length);
        trmLogfn("dev=%p", dev);
        if (!dev) {
            length++;
            continue;
        }

        partitions = partGetPartitions(dev);
        disk = mmAllocKernelObject(struct pt_disk);
        
        disk->Partitions = partitions;
        disk->DeviceIndex = length;
        disk->Ops = dev;
        vectorInsert(&disks, disk);

        length++;
    }
}

struct vector* partGetDisks()
{
    return &disks;
}
