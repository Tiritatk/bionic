#include "../include/ata.h"
#include "../include/io.h"
#include "../include/kprintf.h"

#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_CTRL    0x3F6

#define ATA_REG_DATA        0x00
#define ATA_REG_ERROR       0x01
#define ATA_REG_SECCOUNT0   0x02
#define ATA_REG_LBA0        0x03
#define ATA_REG_LBA1        0x04
#define ATA_REG_LBA2        0x05
#define ATA_REG_HDDEVSEL    0x06
#define ATA_REG_COMMAND     0x07
#define ATA_REG_STATUS      0x07

#define ATA_CMD_IDENTIFY    0xEC
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_CACHE_FLUSH 0xE7

#define ATA_SR_BSY          0x80
#define ATA_SR_DRDY         0x40
#define ATA_SR_DF           0x20
#define ATA_SR_DRQ          0x08
#define ATA_SR_ERR          0x01

static uint8_t ata_present[2] = {0, 0};
static uint32_t ata_sectors[2] = {0, 0};

static void ata_io_wait(void) {
    /*
       Leer varias veces el status crea una pequeña espera.
    */
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
}

static int ata_wait_not_busy(void) {
    uint32_t timeout = 1000000;

    while (timeout--) {
        uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);

        if (!(status & ATA_SR_BSY)) {
            return 0;
        }
    }

    return -1;
}

static int ata_wait_drq(void) {
    uint32_t timeout = 1000000;

    while (timeout--) {
        uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);

        if (status & ATA_SR_ERR) {
            return -1;
        }

        if (status & ATA_SR_DF) {
            return -1;
        }

        if (status & ATA_SR_DRQ) {
            return 0;
        }
    }

    return -1;
}

void ata_init(void) {
    ata_present[0] = 0;
    ata_present[1] = 0;
    ata_sectors[0] = 0;
    ata_sectors[1] = 0;

    if (ata_identify_drive(ATA_DRIVE_MASTER) == 0) {
        ata_present[ATA_DRIVE_MASTER] = 1;
        kprintf("ATA: primary master detected\n");
    } else {
        kprintf("ATA: primary master not detected\n");
    }

    if (ata_identify_drive(ATA_DRIVE_SLAVE) == 0) {
        ata_present[ATA_DRIVE_SLAVE] = 1;
        kprintf("ATA: primary slave detected\n");
    } else {
        kprintf("ATA: primary slave not detected\n");
    }
}

int ata_identify(void) {
    return ata_identify_drive(ATA_DRIVE_MASTER);
}

int ata_identify_drive(uint8_t drive) {
    if (drive > 1) {
        return -1;
    }

    /*
       0xA0 = master
       0xB0 = slave
    */
    uint8_t drive_select = drive == ATA_DRIVE_MASTER ? 0xA0 : 0xB0;

    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, drive_select);
    ata_io_wait();

    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, 0);

    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_io_wait();

    uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);

    if (status == 0) {
        return -1;
    }

    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    uint8_t lba1 = inb(ATA_PRIMARY_IO + ATA_REG_LBA1);
    uint8_t lba2 = inb(ATA_PRIMARY_IO + ATA_REG_LBA2);

    if (lba1 != 0 || lba2 != 0) {
        return -1;
    }

    if (ata_wait_drq() != 0) {
        return -1;
    }

    uint16_t identify[256];

    for (uint32_t i = 0; i < 256; i++) {
        identify[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }

    ata_sectors[drive] = ((uint32_t)identify[61] << 16) | identify[60];

    return 0;
}

int ata_read_sector(uint32_t lba, uint8_t* buffer) {
    return ata_read_sector_drive(ATA_DRIVE_MASTER, lba, buffer);
}

int ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    return ata_write_sector_drive(ATA_DRIVE_MASTER, lba, buffer);
}

uint8_t ata_is_present(void) {
    return ata_present[ATA_DRIVE_MASTER];
}

uint32_t ata_total_sectors(void) {
    return ata_sectors[ATA_DRIVE_MASTER];
}

uint32_t ata_size_mb(void) {
    return ata_sectors[ATA_DRIVE_MASTER] / 2048;
}

uint8_t ata_is_present_drive(uint8_t drive) {
    if (drive > 1) {
        return 0;
    }

    return ata_present[drive];
}

uint32_t ata_total_sectors_drive(uint8_t drive) {
    if (drive > 1) {
        return 0;
    }

    return ata_sectors[drive];
}

uint32_t ata_size_mb_drive(uint8_t drive) {
    if (drive > 1) {
        return 0;
    }

    return ata_sectors[drive] / 2048;
}

int ata_read_sector_drive(uint8_t drive, uint32_t lba, uint8_t* buffer) {
    if (!buffer) {
        return -1;
    }

    if (drive > 1) {
        return -1;
    }

    if (!ata_present[drive]) {
        return -1;
    }

    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    uint8_t drive_bits = drive == ATA_DRIVE_MASTER ? 0xE0 : 0xF0;

    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL,
         drive_bits | ((lba >> 24) & 0x0F));

    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 1);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));

    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    if (ata_wait_drq() != 0) {
        return -1;
    }

    uint16_t* buffer16 = (uint16_t*)buffer;

    for (uint32_t i = 0; i < 256; i++) {
        buffer16[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }

    return 0;
}

int ata_write_sector_drive(uint8_t drive, uint32_t lba, const uint8_t* buffer) {
    if (!buffer) {
        return -1;
    }

    if (drive > 1) {
        return -1;
    }

    if (!ata_present[drive]) {
        return -1;
    }

    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    uint8_t drive_bits = drive == ATA_DRIVE_MASTER ? 0xE0 : 0xF0;

    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL,
         drive_bits | ((lba >> 24) & 0x0F));

    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 1);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));

    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    if (ata_wait_drq() != 0) {
        return -1;
    }

    const uint16_t* buffer16 = (const uint16_t*)buffer;

    for (uint32_t i = 0; i < 256; i++) {
        outw(ATA_PRIMARY_IO + ATA_REG_DATA, buffer16[i]);
    }

    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);

    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    return 0;
}