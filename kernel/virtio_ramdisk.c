#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "buf.h"

extern unsigned char fs_img[];
extern unsigned int fs_img_len;

void
virtio_disk_init(void)
{
    if (fs_img_len == 0)
        panic("virtio_ramdisk: empty image");
}

void
virtio_disk_rw(struct buf *b, int write)
{
    uint64 offset = (uint64)b->blockno * BSIZE;

    if (offset + BSIZE > fs_img_len)
        panic("virtio_ramdisk: out of bounds");

    unsigned char *disk_addr = fs_img + offset;

    if (write) {
        memmove(disk_addr, b->data, BSIZE);
    } else {
        memmove(b->data, disk_addr, BSIZE);
    }

    b->valid = 1;
    b->disk = 0;
    wakeup(b);
}

void
virtio_disk_intr(void)
{
    // no interrupt-driven completions for a synchronous ramdisk
}