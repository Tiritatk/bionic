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

static uint8_t ata_present = 0;
static uint32_t ata_sectors = 0;

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
    ata_present = 0;
    ata_sectors = 0;

    if (ata_identify() == 0) {
        ata_present = 1;
        kprintf("ATA: disco detectado\n");
    } else {
        kprintf("ATA: no se detecto disco\n");
    }
}

int ata_identify(void) {
    /*
       Seleccionar master drive.
    */
    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xA0);
    ata_io_wait();

    /*
       Limpiar registros.
    */
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, 0);

    /*
       Enviar IDENTIFY.
    */
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

    /*
       Si estos no son 0, probablemente no es ATA normal.
    */
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

/*
   Words 60-61 = total LBA28 sectors.
*/
ata_sectors = ((uint32_t)identify[61] << 16) | identify[60];

return 0;
}

int ata_read_sector(uint32_t lba, uint8_t* buffer) {
    if (!buffer) {
        return -1;
    }

    if (!ata_present) {
        return -1;
    }

    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    /*
       Modo LBA28, master.
    */
    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL,
         0xE0 | ((lba >> 24) & 0x0F));

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

int ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    if (!buffer) {
        return -1;
    }

    if (!ata_present) {
        return -1;
    }

    if (ata_wait_not_busy() != 0) {
        return -1;
    }

    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL,
         0xE0 | ((lba >> 24) & 0x0F));

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

uint8_t ata_is_present(void) {
    return ata_present;
}

uint32_t ata_total_sectors(void) {
    return ata_sectors;
}

uint32_t ata_size_mb(void) {
    /*
       1 sector = 512 bytes.
       1 MiB = 1024 * 1024 bytes.
       2048 sectores = 1 MiB.
    */
    return ata_sectors / 2048;
}