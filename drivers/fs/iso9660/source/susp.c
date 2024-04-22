#include "susp.h"
#include "isodef.h"
#include "memory/physical.h"
#include "term/terminal.h"

struct susp_new_state suspHandleEntry(struct iso9660_directory_record* record)
{
    struct susp_new_state ns;
    ns.FileNameLength = 0;

    size_t rec = sizeofrec(record);
    size_t fullSize = record->Size - rec;
    uint8_t* s = PaAdd(record, rec);

    struct susp_sue_header* sue = (struct susp_sue_header*)s;
    while (fullSize > 1) {
        if (sue->Signature[0] == 'N'
         && sue->Signature[1] == 'M') {
            struct susp_alternate_name* nm = (struct susp_alternate_name*)sue;
            size_t stringLength = nm->Header.Length - sizeof(struct susp_alternate_name);
            ns.FileName = PaAdd(nm, sizeof(struct susp_alternate_name));
            ns.FileNameLength = stringLength;
        }

        fullSize -= sue->Length;
        sue = PaAdd(sue, sue->Length);
    }

    return ns;
}
