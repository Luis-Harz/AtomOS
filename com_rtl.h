#include <stdint.h>
#include "vga.h"
#include "libc.h"
#include "pci.h"
#include "timer.h"
static uint8_t my_mac[6]  = {0x52,0x54,0x00,0x12,0x34,0x56}; // NIC's MAC
static uint8_t my_ip[4]      = {10, 0, 2, 15};
static uint8_t arp_mac[6] = {0};
#define RTL_REG_MSR 0x3E
#define RTL_MSR_LINK 0x04

//RTL Cable Check
int cable_in() {
    uint8_t msr = inb(RTL_REG_MSR);
    if (msr & RTL_MSR_LINK) {
        return 1;
    } else {
        return 0;
    }
}

//--------RTL8139-----------
#define RTL_RESET 0x37
#define RTL_CMD   0x37
#define RTL_CFG_OK 0x10
#define RX_BUF_SIZE 8192 + 16 + 1500
#define TX_BUF_SIZE 1536
uint8_t rx_buffer[RX_BUF_SIZE] __attribute__((aligned(4)));

void rtl_init(pci_device_t *rtl) {
    pci_enable_io(rtl);
    pci_enable_busmaster(rtl);
    uint32_t base = rtl->bars[0].base;
    // Power on
    outb(base + 0x52, 0x00);
    // Soft reset
    outb(base + 0x37, 0x10);
    while (inb(base + 0x37) & 0x10);
    // Clear all TX descriptors
    _outl(base + 0x10, 0);
    _outl(base + 0x14, 0);
    _outl(base + 0x18, 0);
    _outl(base + 0x1C, 0);
    // Set up RX buffer
    _outl(base + 0x30, (uint32_t)rx_buffer);
    // Clear IMR/ISR
    _outw(base + 0x3C, 0x0000);
    _outw(base + 0x3E, 0xFFFF);
    _outl(base + 0x44, 0xF | (1 << 7) | (7 << 8));
    // Enable Rx + Tx
    outb(base + 0x37, 0x0C);
    // Set IMR
    _outw(base + 0x3C, 0x0005);
    for (int i = 0; i < 6; i++)
        my_mac[i] = inb(base + i);

    vga_print("\n");
}

static uint32_t tsad[4] = {0x20, 0x24, 0x28, 0x2C};
static uint32_t tsd[4]  = {0x10, 0x14, 0x18, 0x1C};
uint8_t tx_buffer[4][TX_BUF_SIZE] __attribute__((aligned(4)));
static int tx_slot = 0;

int rtl_send_packet(pci_device_t *rtl, uint8_t *data, uint16_t len) {
    uint32_t base = rtl->bars[0].base;
    uint32_t tsd_val = _inl(base + tsd[tx_slot]);
    if (tsd_val != 0) {
        while (!(_inl(base + tsd[tx_slot]) & 0x8000));
    }

    for (int i = 0; i < len; i++)
        tx_buffer[tx_slot][i] = data[i];

    _outl(base + tsad[tx_slot], (uint32_t)tx_buffer[tx_slot]);
    _outl(base + tsd[tx_slot], len & 0x1FFF);
    tx_slot = (tx_slot + 1) % 4;
    return 0;
}

static uint16_t rx_offset = 0;

int rtl_receive_packet(pci_device_t *rtl, uint8_t *buf) {
    uint32_t base = rtl->bars[0].base;
    if ((_inw(base + 0x3A) == _inw(base + 0x38) + 16) ||
        (inb(base + 0x37) & 0x01)) return 0;

    uint8_t  *rx_ptr = rx_buffer + rx_offset;
    uint16_t status  = *(uint16_t *)(rx_ptr);
    uint16_t length  = *(uint16_t *)(rx_ptr + 2);
    if (!(status & 0x01)) return 0;
    for (int i = 0; i < length - 4; i++)
        buf[i] = rx_ptr[4 + i];

    rx_offset = (rx_offset + length + 4 + 3) & ~3;
    rx_offset %= RX_BUF_SIZE;
    _outw(base + 0x38, rx_offset - 16);
    return length - 4;
}
//----------Network----------
#pragma pack(1)
typedef struct {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t ethertype;
} eth_hdr_t;

typedef struct {
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t oper;
    uint8_t  sha[6];
    uint8_t  spa[4];
    uint8_t  tha[6];
    uint8_t  tpa[4];
} arp_hdr_t;

typedef struct {
    uint8_t  ihl_ver;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint8_t  src[4];
    uint8_t  dst[4];
} ip_hdr_t;

typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} icmp_hdr_t;
#pragma pack()

uint16_t checksum(void *data, int len) {
    uint16_t *ptr = (uint16_t*)data;
    uint32_t sum = 0;
    while (len > 1) { sum += *ptr++; len -= 2; }
    if (len) sum += *(uint8_t *)ptr;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

int arp_resolve(pci_device_t *rtl, uint8_t *target_ip) {
    uint8_t pkt[sizeof(eth_hdr_t) + sizeof(arp_hdr_t)];
    eth_hdr_t *eth = (eth_hdr_t *)pkt;
    arp_hdr_t *arp = (arp_hdr_t *)(pkt + sizeof(eth_hdr_t));

    memset(eth->dst, 0xFF, 6);
    memcpy(eth->src, my_mac, 6);
    eth->ethertype = htons(0x0806);

    arp->htype = htons(1);
    arp->ptype = htons(0x0800);
    arp->hlen  = 6;
    arp->plen  = 4;
    arp->oper  = htons(1);
    memcpy(arp->sha, my_mac, 6);
    memcpy(arp->spa, my_ip, 4);
    memset(arp->tha, 0x00, 6);
    memcpy(arp->tpa, target_ip, 4);

    rtl_send_packet(rtl, pkt, sizeof(eth_hdr_t) + sizeof(arp_hdr_t));
    vga_print("ARP request sent\n");

    uint8_t buf[2048];
    for (int tries = 0; tries < 100000; tries++) {
        int len = rtl_receive_packet(rtl, buf);
        if (len < (int)(sizeof(eth_hdr_t) + sizeof(arp_hdr_t))) continue;

        eth_hdr_t *reth = (eth_hdr_t *)buf;
        arp_hdr_t *rarp = (arp_hdr_t *)(buf + sizeof(eth_hdr_t));

        // compare against htons constant, not ntohs of field
        if (reth->ethertype != htons(0x0806)) continue;
        if (rarp->oper != htons(2)) continue;  // 2 = reply
        if (memcmp(rarp->spa, target_ip, 4) != 0) continue;

        memcpy(arp_mac, rarp->sha, 6);
        vga_print("ARP resolved: ");
        for (int i = 0; i < 6; i++) {
            print_hex(arp_mac[i], 2);
            if (i < 5) vga_print(":");
        }
        vga_print("\n");
        return 1;
    }
    return 0;
}


//--------Ping--------
static uint8_t gateway_ip[4] = {10, 0, 2, 2};

int ping(pci_device_t *rtl, uint8_t *target_ip) {
    if (!arp_resolve(rtl, gateway_ip)) {
        vga_print("ARP failed\n");
        return -1;
    }
    uint8_t pkt[sizeof(eth_hdr_t) + sizeof(ip_hdr_t) + sizeof(icmp_hdr_t) + 32];
    memset(pkt, 0, sizeof(pkt));
    eth_hdr_t  *eth  = (eth_hdr_t *)pkt;
    ip_hdr_t   *ip   = (ip_hdr_t  *)(pkt + sizeof(eth_hdr_t));
    icmp_hdr_t *icmp = (icmp_hdr_t *)(pkt + sizeof(eth_hdr_t) + sizeof(ip_hdr_t));
    memcpy(eth->dst, arp_mac, 6);
    memcpy(eth->src, my_mac, 6);
    eth->ethertype = htons(0x0800);
    ip->ihl_ver    = 0x45;
    ip->ttl        = 64;
    ip->proto      = 1;
    ip->total_len  = htons(sizeof(ip_hdr_t) + sizeof(icmp_hdr_t) + 32);
    memcpy(ip->src, my_ip, 4);
    memcpy(ip->dst, target_ip, 4);
    ip->checksum   = checksum(ip, sizeof(ip_hdr_t));
    icmp->type     = 8;
    icmp->code     = 0;
    icmp->id       = htons(0x1234);
    icmp->seq      = htons(1);
    memset((uint8_t *)icmp + sizeof(icmp_hdr_t), 0xAB, 32);
    icmp->checksum = checksum(icmp, sizeof(icmp_hdr_t) + 32);

    rtl_send_packet(rtl, pkt, sizeof(pkt));
    vga_print("Sent ICMP echo request\n");
    uint8_t buf[2048];
    for (int tries = 0; tries < 100000; tries++) {
        int len = rtl_receive_packet(rtl, buf);
        if (len < (int)(sizeof(eth_hdr_t) + sizeof(ip_hdr_t) + sizeof(icmp_hdr_t))) continue;
        ip_hdr_t   *rip   = (ip_hdr_t  *)(buf + sizeof(eth_hdr_t));
        icmp_hdr_t *ricmp = (icmp_hdr_t *)(buf + sizeof(eth_hdr_t) + sizeof(ip_hdr_t));
        if (rip->proto != 1) continue;
        if (memcmp(rip->src, target_ip, 4) != 0) continue;
        if (ricmp->type != 0) continue;
        vga_print("Got ping reply!\n");
        return 0;
    }

    vga_print("Ping timed out\n");
    return -1;
}

//-----UDP-----
#pragma pack(1)
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} udp_hdr_t;
#pragma pack()

void udp_send(pci_device_t *rtl, uint8_t *dst_mac, uint8_t *dst_ip,
              uint16_t src_port, uint16_t dst_port,
              uint8_t *payload, uint16_t payload_len) {

    uint16_t udp_len  = sizeof(udp_hdr_t) + payload_len;
    uint16_t ip_len   = sizeof(ip_hdr_t) + udp_len;
    uint16_t total    = sizeof(eth_hdr_t) + ip_len;

    uint8_t pkt[total];
    memset(pkt, 0, total);

    eth_hdr_t *eth = (eth_hdr_t *)pkt;
    ip_hdr_t  *ip  = (ip_hdr_t  *)(pkt + sizeof(eth_hdr_t));
    udp_hdr_t *udp = (udp_hdr_t *)(pkt + sizeof(eth_hdr_t) + sizeof(ip_hdr_t));
    uint8_t   *data = pkt + sizeof(eth_hdr_t) + sizeof(ip_hdr_t) + sizeof(udp_hdr_t);

    memcpy(eth->dst, dst_mac, 6);
    memcpy(eth->src, my_mac, 6);
    eth->ethertype = htons(0x0800);

    ip->ihl_ver   = 0x45;
    ip->ttl       = 64;
    ip->proto     = 17; // UDP
    ip->total_len = htons(ip_len);
    memcpy(ip->src, my_ip, 4);
    memcpy(ip->dst, dst_ip, 4);
    ip->checksum  = checksum(ip, sizeof(ip_hdr_t));

    udp->src_port = htons(src_port);
    udp->dst_port = htons(dst_port);
    udp->length   = htons(udp_len);
    udp->checksum = 0;

    memcpy(data, payload, payload_len);

    rtl_send_packet(rtl, pkt, total);
}
//-----DHCP-----
#pragma pack(1)
typedef struct {
    uint8_t  op;
    uint8_t  htype;
    uint8_t  hlen;
    uint8_t  hops;
    uint32_t xid;         // transaction ID
    uint16_t secs;
    uint16_t flags;
    uint8_t  ciaddr[4];   // client IP
    uint8_t  yiaddr[4];   // offered IP
    uint8_t  siaddr[4];   // server IP
    uint8_t  giaddr[4];   // gateway IP
    uint8_t  chaddr[16];  // client MAC
    uint8_t  sname[64];
    uint8_t  file[128];
    uint32_t magic;
    uint8_t  options[64];
} dhcp_pkt_t;
#pragma pack()

#define DHCP_MAGIC 0x63825363

static uint8_t broadcast_mac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static uint8_t zero_ip[4]       = {0,0,0,0};
static uint8_t broadcast_ip[4]  = {255,255,255,255};

int dhcp_request(pci_device_t *rtl) {
    dhcp_pkt_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.op    = 1;
    pkt.htype = 1;
    pkt.hlen  = 6;
    pkt.xid   = htonl(0xDEADBEEF);
    pkt.flags = htons(0x8000);
    pkt.magic = htonl(DHCP_MAGIC);
    memcpy(pkt.chaddr, my_mac, 6);
    int i = 0;
    pkt.options[i++] = 53;  // DHCP message type
    pkt.options[i++] = 1;   // length
    pkt.options[i++] = 1;   // DISCOVER
    pkt.options[i++] = 55;  // option: parameter request list
    pkt.options[i++] = 3;   // length
    pkt.options[i++] = 1;   // subnet mask
    pkt.options[i++] = 3;   // router
    pkt.options[i++] = 6;   // DNS
    pkt.options[i++] = 255; // end
    udp_send(rtl, broadcast_mac, broadcast_ip,
             68, 67, (uint8_t *)&pkt, sizeof(pkt));
    vga_print("DHCP DISCOVER sent\n");
    uint8_t buf[2048];
    for (int tries = 0; tries < 200000; tries++) {
        int len = rtl_receive_packet(rtl, buf);
        if (len < (int)(sizeof(eth_hdr_t) + sizeof(ip_hdr_t) +
                        sizeof(udp_hdr_t) + sizeof(dhcp_pkt_t))) continue;
        udp_hdr_t  *udp  = (udp_hdr_t  *)(buf + sizeof(eth_hdr_t) + sizeof(ip_hdr_t));
        dhcp_pkt_t *resp = (dhcp_pkt_t *)(buf + sizeof(eth_hdr_t) + sizeof(ip_hdr_t) + sizeof(udp_hdr_t));
        if (udp->dst_port != htons(68)) continue;
        if (resp->op != 2) continue;
        if (resp->xid != htonl(0xDEADBEEF)) continue;
        uint8_t *opt = resp->options;
        uint8_t msg_type = 0;
        while (opt < resp->options + sizeof(resp->options) && *opt != 255) {
            if (*opt == 53) { msg_type = opt[2]; break; }
            opt += 2 + opt[1];
        }
        if (msg_type != 2) continue;
        vga_print("DHCP OFFER received, offered IP: ");
        for (int j = 0; j < 4; j++) {
            print_dec(resp->yiaddr[j]);
            if (j < 3) vga_print(".");
        }
        vga_print("\n");
        memset(pkt.options, 0, sizeof(pkt.options));
        i = 0;
        pkt.options[i++] = 53;
        pkt.options[i++] = 1;
        pkt.options[i++] = 3;
        pkt.options[i++] = 50;
        pkt.options[i++] = 4;
        memcpy(&pkt.options[i], resp->yiaddr, 4); i += 4;
        pkt.options[i++] = 255;
        udp_send(rtl, broadcast_mac, broadcast_ip,
                 68, 67, (uint8_t *)&pkt, sizeof(pkt));
        vga_print("DHCP REQUEST sent\n");
        for (int tries2 = 0; tries2 < 200000; tries2++) {
            len = rtl_receive_packet(rtl, buf);
            if (len < (int)(sizeof(eth_hdr_t) + sizeof(ip_hdr_t) +
                            sizeof(udp_hdr_t) + sizeof(dhcp_pkt_t))) continue;

            udp  = (udp_hdr_t  *)(buf + sizeof(eth_hdr_t) + sizeof(ip_hdr_t));
            resp = (dhcp_pkt_t *)(buf + sizeof(eth_hdr_t) + sizeof(ip_hdr_t) + sizeof(udp_hdr_t));

            if (udp->dst_port != htons(68)) continue;
            if (resp->xid != htonl(0xDEADBEEF)) continue;

            opt = resp->options;
            msg_type = 0;
            while (opt < resp->options + sizeof(resp->options) && *opt != 255) {
                if (*opt == 53) { msg_type = opt[2]; break; }
                opt += 2 + opt[1];
            }
            if (msg_type != 5) continue;
            memcpy(my_ip, resp->yiaddr, 4);
            vga_print("DHCP ACK! IP assigned: ");
            for (int j = 0; j < 4; j++) {
                print_dec(my_ip[j]);
                if (j < 3) vga_print(".");
            }
            vga_print("\n");
            return 1;
        }

        vga_print("DHCP ACK timeout\n");
        return 0;
    }

    vga_print("DHCP OFFER timeout\n");
    return 0;
}

//--------DNS--------
#pragma pack(1)
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_hdr_t;
#pragma pack()

static uint8_t dns_ip[4] = {8, 8, 8, 8};  // Google DNS

int dns_resolve(pci_device_t *rtl, const char *hostname, uint8_t *out_ip) {
    uint8_t pkt[512];
    memset(pkt, 0, sizeof(pkt));
    dns_hdr_t *hdr = (dns_hdr_t *)pkt;
    hdr->id      = htons(0x1234);
    hdr->flags   = htons(0x0100);
    hdr->qdcount = htons(1);
    uint8_t *qptr = pkt + sizeof(dns_hdr_t);
    const char *p = hostname;
    while (*p) {
        const char *dot = p;
        while (*dot && *dot != '.') dot++;
        *qptr++ = (uint8_t)(dot - p);
        while (p < dot) *qptr++ = *p++;
        if (*p == '.') p++;
    }
    *qptr++ = 0;
    *qptr++ = 0; *qptr++ = 1;
    *qptr++ = 0; *qptr++ = 1;
    uint16_t pkt_len = qptr - pkt;
    if (!arp_resolve(rtl, gateway_ip)) {
        vga_print("DNS: ARP failed\n");
        return 0;
    }

    udp_send(rtl, arp_mac, dns_ip, 1234, 53, pkt, pkt_len);
    vga_print("DNS query sent\n");
    uint8_t buf[2048];
    for (int tries = 0; tries < 200000; tries++) {
        int len = rtl_receive_packet(rtl, buf);
        if (len < (int)(sizeof(eth_hdr_t) + sizeof(ip_hdr_t) +
                        sizeof(udp_hdr_t) + sizeof(dns_hdr_t))) continue;

        udp_hdr_t *udp  = (udp_hdr_t *)(buf + sizeof(eth_hdr_t) + sizeof(ip_hdr_t));
        if (udp->dst_port != htons(1234)) continue; 

        uint8_t *resp = buf + sizeof(eth_hdr_t) + sizeof(ip_hdr_t) + sizeof(udp_hdr_t);
        dns_hdr_t *rhdr = (dns_hdr_t *)resp;
        if (rhdr->id != htons(0x1234)) continue;
        if (ntohs(rhdr->ancount) == 0) {
            vga_print("DNS: no answers\n");
            return 0;
        }

        uint8_t *ptr = resp + sizeof(dns_hdr_t);
        while (*ptr != 0) {
            if ((*ptr & 0xC0) == 0xC0) { ptr += 2; goto skip_done; }
            ptr += 1 + *ptr;
        }
        ptr++;
        skip_done:
        ptr += 4;
        for (int a = 0; a < ntohs(rhdr->ancount); a++) {
            if ((*ptr & 0xC0) == 0xC0) ptr += 2;
            else { while (*ptr) { ptr += 1 + *ptr; } ptr++; }

            uint16_t rtype  = (*ptr << 8) | *(ptr+1); ptr += 2;
            ptr += 2;
            ptr += 4;
            uint16_t rdlen  = (*ptr << 8) | *(ptr+1); ptr += 2;

            if (rtype == 1 && rdlen == 4) {
                memcpy(out_ip, ptr, 4);
                vga_print("DNS resolved: ");
                for (int i = 0; i < 4; i++) {
                    print_dec(out_ip[i]);
                    if (i < 3) vga_print(".");
                }
                vga_print("\n");
                return 1;
            }
            ptr += rdlen;
        }
        vga_print("DNS: no A record found\n");
        return 0;
    }

    vga_print("DNS timeout\n");
    return 0;
}

//-----Own Network Protocol-----
//Name: TinyProtocol

#define TINY_PORT 2486

//---Header---
#pragma pack(1)
typedef struct {
    uint8_t type;   // 1=connect, 2=ack, 3=data
    uint8_t seq;   //sequence
    uint8_t length;   //length
    uint8_t data[256];   //data
} tiny_pkt_t;
#pragma pack()

//Sending
void tiny_send(pci_device_t *rtl, uint8_t* dst_mac, uint8_t* dst_ip, uint8_t type, uint8_t seq, uint8_t* data, uint8_t len) {
    tiny_pkt_t pkt;
    pkt.type = type;
    pkt.seq = seq;
    pkt.length = len;
    if (len > 0) {
        memcpy(pkt.data, data, len);
    }
    udp_send(rtl, dst_mac, dst_ip, TINY_PORT, TINY_PORT, (uint8_t*)&pkt, sizeof(tiny_pkt_t));
}

//Receiving
int tiny_receive(pci_device_t *rtl ,tiny_pkt_t* out) {
    uint8_t buf[2048];
    int len = rtl_receive_packet(rtl, buf);
    if (len <= 0) return 0;
    eth_hdr_t* eth = (eth_hdr_t*)buf;
    if (eth->ethertype != htons(0x0800)) return 0;
    ip_hdr_t* ip = (ip_hdr_t*)(buf + sizeof(eth_hdr_t));
    if (ip->proto != 17) return 0; // UDP
    udp_hdr_t* udp = (udp_hdr_t*)((uint8_t*)ip + sizeof(ip_hdr_t));
    if (ntohs(udp->dst_port) != TINY_PORT) return 0;
    tiny_pkt_t* pkt = (tiny_pkt_t*)((uint8_t*)udp + sizeof(udp_hdr_t));
    memcpy(out, pkt, sizeof(tiny_pkt_t));
    return 1;
}

int connect_tiny(pci_device_t *rtl ,uint8_t* mac, uint8_t* ip) {
    tiny_pkt_t resp;
    uint32_t start = ms_since_startup();
    tiny_send(rtl, mac, ip, 1, 1, 0, 0);
    int retries = 0;
    while (retries < 5) {
        if (tiny_receive(rtl ,&resp)) {
            if (resp.type == 2 && resp.seq == 1) {
                return 1; // ACK
            }
        }
        if (ms_since_startup() - start > 100) {
            tiny_send(rtl, mac, ip, 1, 1, 0, 0);
            retries++;
            start = ms_since_startup();
        }
        delay_ms(1);
    }
    return 0;
}