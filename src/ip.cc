/*
 * guzhoudiaoke@126.com
 * 2018-03-21
 */

#include "ip.h"
#include "babyos.h"
#include "net.h"
#include "string.h"

void ip_hdr_s::init(uint8 hdr_len, uint8 ver, uint8 tos, uint16 total_len,
        uint16 id, uint16 offset, uint8 ttl, uint8 proto, uint16 check_sum, 
        uint32 src_ip, uint32 dst_ip)
{
    m_header_len = hdr_len;
    m_version    = ver;
    m_tos        = tos;
    m_total_len  = total_len;
    m_id         = id;
    m_offset     = offset;
    m_ttl        = ttl;
    m_protocol   = proto;
    m_check_sum  = check_sum;
    m_src_ip     = src_ip;
    m_dst_ip     = dst_ip;
}

/**********************************************************/

void ip_t::init()
{
    m_next_id = 0;
}

bool ip_t::is_broadcast(uint32 ip)
{
    if (ip == 0xffffffff || ip == 0x00000000) {
        return true;
    }

    uint32 mask = os()->get_net()->get_subnet_mask();
    if ((ip & ~mask) == (0xffffffff & ~mask)) {
        return true;
    }

    return false;
}

bool ip_t::check_ip(uint32 ip)
{
    if (is_broadcast(ip)) {
        console()->kprintf(RED, "DO NOT support broadcast now\n");
        return false;
    }

    if (ip == os()->get_net()->get_ipaddr()) {
        console()->kprintf(RED, "DO NOT support loopback now\n");
        return false;
    }

    return true;
}

bool ip_t::is_same_subnet(uint32 ip1, uint32 ip2, uint32 mask)
{
    return (ip1 & mask) == (ip2 & mask);
}

void ip_t::transmit(uint32 ip, uint8* data, uint32 len, uint8 protocol)
{
    if (!check_ip(ip)) {
        return;
    }

    uint32 dst_ip = ip;
    if (!is_same_subnet(ip, os()->get_net()->get_ipaddr(), os()->get_net()->get_subnet_mask())) {
        /* if not in same subnet, send to gateway */
        dst_ip = os()->get_net()->get_gateway();
    }

    uint32 total = len + sizeof(ip_hdr_t);
    net_buf_t* buffer = os()->get_net()->alloc_net_buffer(total);
    if (buffer == NULL) {
        console()->kprintf(RED, "allocate net buffer failed!\n");
        return;
    }

    ip_hdr_t hdr;
    hdr.init(sizeof(ip_hdr_t) / 4,                  /* header len */
             0x4,                                   /* version */
             0,                                     /* tos */
             htons(total),                          /* total len */
             htons(m_next_id++),                    /* id */
             htons(0),                              /* offset, don't fragment */
             32,                                    /* ttl */
             protocol,                              /* protocol */
             0,                                     /* check sum */
             htonl(os()->get_net()->get_ipaddr()),  /* source ip */
             htonl(dst_ip));                        /* dest ip */

    /* calc check sum */
    hdr.m_check_sum = net_t::check_sum((uint8 *) (&hdr), sizeof(ip_hdr_t));

    buffer->m_data = (uint8 *) buffer + sizeof(net_buf_t);
    buffer->m_data_len = total;
    uint8* p = buffer->m_data;
    memcpy(p, &hdr, sizeof(ip_hdr_t));

    p += sizeof(ip_hdr_t);
    memcpy(p, data, len);

    uint8 eth_addr[ETH_ADDR_LEN] = {0};
    if (os()->get_net()->get_arp()->lookup_cache(dst_ip, eth_addr)) {
        console()->kprintf(YELLOW, "find mac addr of ip in arp cache\n");
        os()->get_net()->get_ethernet()->transmit(eth_addr, PROTO_IP, buffer->m_data, buffer->m_data_len);
        os()->get_net()->free_net_buffer(buffer);
    }
    else {
        console()->kprintf(YELLOW, "not find mac addr of ip in arp cache\n");
        os()->get_net()->get_arp()->add_to_wait_queue(dst_ip, buffer);
    }
}

void ip_t::receive(net_buf_t* buf)
{
    ip_hdr_t* hdr = (ip_hdr_t *) buf->m_data;
    buf->m_data += sizeof(ip_hdr_t);

    if (net_t::check_sum((uint8 *) hdr, sizeof(ip_hdr_t)) != 0) {
        console()->kprintf(RED, "get a ip package, from: ");
        net_t::dump_ip_addr(hdr->m_src_ip);
        console()->kprintf(RED, " but it's checksum is wrong, drop it\n");
        return;
    }

    if (hdr->m_protocol == 0xff) {
        console()->kprintf(YELLOW, "get a raw ip package, data: %s\n", buf->m_data);
    }
    else {
        console()->kprintf(YELLOW, "get a non-raw ip package protocol: %x\n", hdr->m_protocol);
    }
}

