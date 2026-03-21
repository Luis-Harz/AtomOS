#ifndef PCI_DRIVER_H
#define PCI_DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "vga.h"

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* =====================================================================
 * PCI CONFIGURATION SPACE
 * ===================================================================== */

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_ADDR(bus, dev, func, reg) \
    ( (u32)(1u << 31)                 | \
      ((u32)(bus)  & 0xFFu)  << 16    | \
      ((u32)(dev)  & 0x1Fu)  << 11    | \
      ((u32)(func) & 0x07u)  << 8     | \
      ((u32)(reg)  & 0xFCu) )

/* Standard config offsets */
#define PCI_REG_VENDOR_ID     0x00
#define PCI_REG_DEVICE_ID     0x02
#define PCI_REG_COMMAND       0x04
#define PCI_REG_STATUS        0x06
#define PCI_REG_REVISION      0x08
#define PCI_REG_PROG_IF       0x09
#define PCI_REG_SUBCLASS      0x0A
#define PCI_REG_CLASS         0x0B
#define PCI_REG_HEADER_TYPE   0x0E
#define PCI_REG_BAR0          0x10
#define PCI_REG_BAR1          0x14
#define PCI_REG_BAR2          0x18
#define PCI_REG_BAR3          0x1C
#define PCI_REG_BAR4          0x20
#define PCI_REG_BAR5          0x24
#define PCI_REG_SUBSYS_VENDOR 0x2C
#define PCI_REG_SUBSYS_ID     0x2E
#define PCI_REG_IRQ_LINE      0x3C
#define PCI_REG_IRQ_PIN       0x3D

/* COMMAND bits */
#define PCI_CMD_IO_SPACE    (1u << 0)
#define PCI_CMD_MEM_SPACE   (1u << 1)
#define PCI_CMD_BUS_MASTER  (1u << 2)

/* HEADER TYPE */
#define PCI_HEADER_NORMAL     0x00
#define PCI_HEADER_PCI_BRIDGE 0x01
#define PCI_HEADER_CARDBUS    0x02
#define PCI_HEADER_MULTI_FUNC 0x80

#define PCI_VENDOR_NONE 0xFFFF

/* Bus limits */
#define PCI_MAX_BUS      256
#define PCI_MAX_DEV      32
#define PCI_MAX_FUNC     8
#define PCI_MAX_BARS     6
#define PCI_MAX_DEVICES  64

/* =====================================================================
 * BAR DESCRIPTOR
 * ===================================================================== */

typedef enum {
    PCI_BAR_NONE = 0,
    PCI_BAR_IO,
    PCI_BAR_MEM32,
    PCI_BAR_MEM64,
} pci_bar_type_t;

typedef struct {
    pci_bar_type_t type;
    u64            base;     /* physical or I/O port */
    u64            size;     /* bytes */
    bool           prefetch;
} pci_bar_t;

/* =====================================================================
 * DEVICE DESCRIPTOR
 * ===================================================================== */

typedef struct {
    u8  bus;
    u8  device;
    u8  function;

    u16 vendor_id;
    u16 device_id;
    u16 subsys_vendor;
    u16 subsys_id;
    u8  class_code;
    u8  subclass;
    u8  prog_if;
    u8  revision;

    pci_bar_t bars[PCI_MAX_BARS];

    u8  irq_line;
    u8  irq_pin;

    bool valid;
    bool multifunction;
} pci_device_t;

typedef struct {
    pci_device_t devices[PCI_MAX_DEVICES];
    size_t       count;
} pci_bus_t;

/* =====================================================================
 * LOW-LEVEL I/O
 * ===================================================================== */

static inline void _outl(u16 port, u32 val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline u32 _inl(u16 port) {
    u32 val;
    __asm__ volatile ("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void _outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t _inw(uint16_t port) {
    uint16_t val;
    __asm__ volatile ("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* =====================================================================
 * CONFIG READ/WRITE
 * ===================================================================== */

static inline u32 pci_cfg_read32(u8 bus, u8 dev, u8 func, u8 reg) {
    _outl(PCI_CONFIG_ADDRESS, PCI_ADDR(bus, dev, func, reg));
    return _inl(PCI_CONFIG_DATA);
}

static inline u16 pci_cfg_read16(u8 bus, u8 dev, u8 func, u8 reg) {
    u32 dword = pci_cfg_read32(bus, dev, func, reg & ~0x3u);
    return (u16)(dword >> ((reg & 2u) * 8u));
}

static inline u8 pci_cfg_read8(u8 bus, u8 dev, u8 func, u8 reg) {
    u32 dword = pci_cfg_read32(bus, dev, func, reg & ~0x3u);
    return (u8)(dword >> ((reg & 3u) * 8u));
}

static inline void pci_cfg_write32(u8 bus, u8 dev, u8 func, u8 reg, u32 val) {
    _outl(PCI_CONFIG_ADDRESS, PCI_ADDR(bus, dev, func, reg));
    _outl(PCI_CONFIG_DATA, val);
}

static inline void pci_cfg_write16(u8 bus, u8 dev, u8 func, u8 reg, u16 val) {
    u32 dword = pci_cfg_read32(bus, dev, func, reg & ~0x3u);
    u32 shift = (reg & 2u) * 8u;
    dword = (dword & ~(0xFFFFu << shift)) | ((u32)val << shift);
    pci_cfg_write32(bus, dev, func, reg & ~0x3u, dword);
}

static inline void pci_cfg_write8(u8 bus, u8 dev, u8 func, u8 reg, u8 val) {
    u32 dword = pci_cfg_read32(bus, dev, func, reg & ~0x3u);
    u32 shift = (reg & 3u) * 8u;
    dword = (dword & ~(0xFFu << shift)) | ((u32)val << shift);
    pci_cfg_write32(bus, dev, func, reg & ~0x3u, dword);
}

/* =====================================================================
 * BAR PROBING
 * ===================================================================== */

static inline bool pci_probe_bar(u8 bus, u8 dev, u8 func,
    u8 idx, pci_bar_t *out) {
    u8  reg  = (u8)(PCI_REG_BAR0 + idx * 4u);
    u32 orig = pci_cfg_read32(bus, dev, func, reg);
    if (orig == 0xFFFFFFFFu) {
        out->type = PCI_BAR_NONE;
        out->base = 0; out->size = 0; out->prefetch = false;
        return false;
    }

    pci_cfg_write32(bus, dev, func, reg, 0xFFFFFFFFu);
    u32 mask = pci_cfg_read32(bus, dev, func, reg);
    pci_cfg_write32(bus, dev, func, reg, orig);

    // if mask is 0 after write-all-ones, BAR truly not implemented
    if (mask == 0 || mask == 0xFFFFFFFFu) {
        out->type = PCI_BAR_NONE;
        out->base = 0; out->size = 0; out->prefetch = false;
        return false;
    }

    if (orig & 0x1u) {
        out->type = PCI_BAR_IO;
        out->base = (u64)(orig & 0xFFFCu);
        u32 size_mask = mask & 0xFFFCu;
        out->size = size_mask ? (u64)(~size_mask + 1u) : 0;
        out->prefetch = false;
    } else {
        u8 width = (u8)((orig >> 1u) & 0x3u);
        out->prefetch = ((orig >> 3u) & 0x1u) != 0;
        if (width == 0x2u) {
            u8  reg_hi  = (u8)(reg + 4u);
            u32 orig_hi = pci_cfg_read32(bus, dev, func, reg_hi);
            pci_cfg_write32(bus, dev, func, reg_hi, 0xFFFFFFFFu);
            u32 mask_hi = pci_cfg_read32(bus, dev, func, reg_hi);
            pci_cfg_write32(bus, dev, func, reg_hi, orig_hi);
            u64 full_mask = ((u64)mask_hi << 32u) | (u64)(mask & 0xFFFFFFF0u);
            out->type = PCI_BAR_MEM64;
            out->base = ((u64)orig_hi << 32u) | (u64)(orig & 0xFFFFFFF0u);
            out->size = full_mask ? ~full_mask + 1u : 0;
        } else {
            u32 addr_mask = mask & 0xFFFFFFF0u;
            out->type = PCI_BAR_MEM32;
            out->base = (u64)(orig & 0xFFFFFFF0u);
            out->size = addr_mask ? (u64)(~addr_mask + 1u) : 0;
        }
    }
    return true;
}

/* =====================================================================
 * DEVICE CONFIG SNAPSHOT
 * ===================================================================== */

static inline bool pci_scan_device(u8 bus, u8 dev,
                                   u8 func, pci_device_t *out) {
    u16 vendor = pci_cfg_read16(bus, dev, func, PCI_REG_VENDOR_ID);
    if (vendor == PCI_VENDOR_NONE)
        return false;

    out->bus      = bus;
    out->device   = dev;
    out->function = func;
    out->valid    = true;

    out->vendor_id     = vendor;
    out->device_id     = pci_cfg_read16(bus, dev, func, PCI_REG_DEVICE_ID);
    out->class_code    = pci_cfg_read8 (bus, dev, func, PCI_REG_CLASS);
    out->subclass      = pci_cfg_read8 (bus, dev, func, PCI_REG_SUBCLASS);
    out->prog_if       = pci_cfg_read8 (bus, dev, func, PCI_REG_PROG_IF);
    out->revision      = pci_cfg_read8 (bus, dev, func, PCI_REG_REVISION);
    out->subsys_vendor = pci_cfg_read16(bus, dev, func, PCI_REG_SUBSYS_VENDOR);
    out->subsys_id     = pci_cfg_read16(bus, dev, func, PCI_REG_SUBSYS_ID);
    out->irq_line      = pci_cfg_read8 (bus, dev, func, PCI_REG_IRQ_LINE);
    out->irq_pin       = pci_cfg_read8 (bus, dev, func, PCI_REG_IRQ_PIN);

    u8 header = pci_cfg_read8(bus, dev, func, PCI_REG_HEADER_TYPE);
    out->multifunction = (header & PCI_HEADER_MULTI_FUNC) != 0;

    /* Clear BARs first */
    for (u8 i = 0; i < PCI_MAX_BARS; i++) {
        out->bars[i].type = PCI_BAR_NONE;
        out->bars[i].base = 0;
        out->bars[i].size = 0;
        out->bars[i].prefetch = false;
    }
    if ((header & 0x7Fu) == PCI_HEADER_NORMAL) {
        for (u8 i = 0; i < PCI_MAX_BARS; ) {
            if (!pci_probe_bar(bus, dev, func, i, &out->bars[i])) {
                i++; continue;
            }
            if (out->bars[i].type == PCI_BAR_MEM64 && i + 1 < PCI_MAX_BARS) {
                out->bars[i + 1].type = PCI_BAR_NONE; // mark high dword as consumed
                i += 2; // skip both lo and hi
            } else {
                i++;
            }
        }
    }

    return true;
}

static inline void zero_mem(void *ptr, u32 size) {
    u8 *p = (u8 *)ptr;
    for (u32 i = 0; i < size; i++) p[i] = 0;
}

static inline void copy_mem(void *dst, const void *src, u32 size) {
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    for (u32 i = 0; i < size; i++) d[i] = s[i];
}

/* =====================================================================
 * BUS ENUMERATION
 * ===================================================================== */

static pci_device_t _pci_tmp;

static void dbg(const char *msg) {
    vga_print(msg);
}

static void dbg_ok(void) {
    vga_print(" OK");
}

static void dbg_fail(void) {
    vga_print(" FAIL");
}

static inline void pci_enumerate(pci_bus_t *out) {
    if (!out) return;
    out->count = 0;
    zero_mem(out->devices, sizeof(out->devices));

    for (u8 bus = 0; bus < 4; bus++) {
        for (u8 dev = 0; dev < PCI_MAX_DEV; dev++) {
            u16 vendor = pci_cfg_read16(bus, dev, 0, PCI_REG_VENDOR_ID);
            if (vendor == 0xFFFFu || vendor == 0x0000u) continue;

            u8 header  = pci_cfg_read8(bus, dev, 0, PCI_REG_HEADER_TYPE);
            u8 max_func = (header & PCI_HEADER_MULTI_FUNC) ? PCI_MAX_FUNC : 1;

            for (u8 func = 0; func < max_func; func++) {
                vendor = pci_cfg_read16(bus, dev, func, PCI_REG_VENDOR_ID);
                if (vendor == 0xFFFFu || vendor == 0x0000u) continue;
                if (out->count >= PCI_MAX_DEVICES) return;

                pci_device_t *d = &out->devices[out->count];
                zero_mem(d, sizeof(*d));
                if (!pci_scan_device(bus, dev, func, d)) continue;
                if (d->vendor_id == 0xFFFFu || d->vendor_id == 0) continue;

                out->count++;

                vga_print("PCI ");
                if (d->vendor_id == 0x8086)      vga_print("Intel");
                else if (d->vendor_id == 0x10DE) vga_print("NVIDIA");
                else if (d->vendor_id == 0x1002) vga_print("AMD");
                else if (d->vendor_id == 0x1234) vga_print("BOCHS");
                else                             vga_print("????");

                vga_print(" class:");
                if      (d->class_code == 0x01) vga_print("storage");
                else if (d->class_code == 0x02) vga_print("net");
                else if (d->class_code == 0x03) vga_print("display");
                else if (d->class_code == 0x06) vga_print("bridge");
                else                            vga_print("?");
                vga_print("\n");
            }
        }
    }
    vga_print("PCI done\n");
}

/* =====================================================================
 * SIMPLE LOOKUPS
 * ===================================================================== */

static inline pci_device_t *pci_find_device(pci_bus_t *bus,
                                            u16 vendor_id, u16 device_id) {
    for (size_t i = 0; i < bus->count; i++) {
        pci_device_t *d = &bus->devices[i];
        if ((vendor_id == 0 || d->vendor_id == vendor_id) &&
            (device_id == 0 || d->device_id == device_id))
            return d;
    }
    return NULL;
}

static inline pci_device_t *pci_find_class(pci_bus_t *bus,
                                           u8 class_code, u8 subclass) {
    for (size_t i = 0; i < bus->count; i++) {
        pci_device_t *d = &bus->devices[i];
        if (d->class_code == class_code && d->subclass == subclass)
            return d;
    }
    return NULL;
}

/* =====================================================================
 * COMMAND HELPERS
 * ===================================================================== */

static inline void pci_enable_io(const pci_device_t *d) {
    u16 cmd = pci_cfg_read16(d->bus, d->device, d->function, PCI_REG_COMMAND);
    pci_cfg_write16(d->bus, d->device, d->function,
                    PCI_REG_COMMAND, cmd | PCI_CMD_IO_SPACE);
}

static inline void pci_enable_mmio(const pci_device_t *d) {
    u16 cmd = pci_cfg_read16(d->bus, d->device, d->function, PCI_REG_COMMAND);
    pci_cfg_write16(d->bus, d->device, d->function,
                    PCI_REG_COMMAND, cmd | PCI_CMD_MEM_SPACE);
}

static inline void pci_enable_busmaster(const pci_device_t *d) {
    u16 cmd = pci_cfg_read16(d->bus, d->device, d->function, PCI_REG_COMMAND);
    pci_cfg_write16(d->bus, d->device, d->function,
                    PCI_REG_COMMAND, cmd | PCI_CMD_BUS_MASTER);
}

#endif /* PCI_DRIVER_H */
