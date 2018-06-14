/* SPDX-License-Identifier: LGPL-2.1+ */
/***
  Copyright © 2014 Susant Sahani
***/

#include <net/if.h>

#include "sd-netlink.h"

#include "conf-parser.h"
#include "alloc-util.h"
#include "extract-word.h"
#include "string-util.h"
#include "strv.h"
#include "parse-util.h"
#include "missing.h"

#include "networkd-link.h"
#include "netdev/vxlan.h"

static int netdev_vxlan_fill_message_create(NetDev *netdev, Link *link, sd_netlink_message *m) {
        VxLan *v;
        int r;

        assert(netdev);
        assert(link);
        assert(m);

        v = VXLAN(netdev);

        assert(v);

        if (v->id <= VXLAN_VID_MAX) {
                r = sd_netlink_message_append_u32(m, IFLA_VXLAN_ID, v->id);
                if (r < 0)
                        return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_ID attribute: %m");
        }

        if (!in_addr_is_null(v->remote_family, &v->remote)) {

                if (v->remote_family == AF_INET)
                        r = sd_netlink_message_append_in_addr(m, IFLA_VXLAN_GROUP, &v->remote.in);
                else
                        r = sd_netlink_message_append_in6_addr(m, IFLA_VXLAN_GROUP6, &v->remote.in6);

                if (r < 0)
                        return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_GROUP attribute: %m");

        }

        if (!in_addr_is_null(v->local_family, &v->local)) {

                if (v->local_family == AF_INET)
                        r = sd_netlink_message_append_in_addr(m, IFLA_VXLAN_LOCAL, &v->local.in);
                else
                        r = sd_netlink_message_append_in6_addr(m, IFLA_VXLAN_LOCAL6, &v->local.in6);

                if (r < 0)
                        return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_LOCAL attribute: %m");

        }

        r = sd_netlink_message_append_u32(m, IFLA_VXLAN_LINK, link->ifindex);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_LINK attribute: %m");

        if (v->ttl) {
                r = sd_netlink_message_append_u8(m, IFLA_VXLAN_TTL, v->ttl);
                if (r < 0)
                        return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_TTL attribute: %m");
        }

        if (v->tos) {
                r = sd_netlink_message_append_u8(m, IFLA_VXLAN_TOS, v->tos);
                if (r < 0)
                        return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_TOS attribute: %m");
        }

        r = sd_netlink_message_append_u8(m, IFLA_VXLAN_LEARNING, v->learning);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_LEARNING attribute: %m");

        r = sd_netlink_message_append_u8(m, IFLA_VXLAN_RSC, v->route_short_circuit);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_RSC attribute: %m");

        r = sd_netlink_message_append_u8(m, IFLA_VXLAN_PROXY, v->arp_proxy);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_PROXY attribute: %m");

        r = sd_netlink_message_append_u8(m, IFLA_VXLAN_L2MISS, v->l2miss);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_L2MISS attribute: %m");

        r = sd_netlink_message_append_u8(m, IFLA_VXLAN_L3MISS, v->l3miss);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_L3MISS attribute: %m");

        if (v->fdb_ageing) {
                r = sd_netlink_message_append_u32(m, IFLA_VXLAN_AGEING, v->fdb_ageing / USEC_PER_SEC);
                if (r < 0)
                        return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_AGEING attribute: %m");
        }

        if (v->max_fdb) {
                r = sd_netlink_message_append_u32(m, IFLA_VXLAN_LIMIT, v->max_fdb);
                if (r < 0)
                        return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_LIMIT attribute: %m");
        }

        r = sd_netlink_message_append_u8(m, IFLA_VXLAN_UDP_CSUM, v->udpcsum);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_UDP_CSUM attribute: %m");

        r = sd_netlink_message_append_u8(m, IFLA_VXLAN_UDP_ZERO_CSUM6_TX, v->udp6zerocsumtx);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_UDP_ZERO_CSUM6_TX attribute: %m");

        r = sd_netlink_message_append_u8(m, IFLA_VXLAN_UDP_ZERO_CSUM6_RX, v->udp6zerocsumrx);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_UDP_ZERO_CSUM6_RX attribute: %m");

        r = sd_netlink_message_append_u8(m, IFLA_VXLAN_REMCSUM_TX, v->remote_csum_tx);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_REMCSUM_TX attribute: %m");

        r = sd_netlink_message_append_u8(m, IFLA_VXLAN_REMCSUM_RX, v->remote_csum_rx);
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_REMCSUM_RX attribute: %m");

        r = sd_netlink_message_append_u16(m, IFLA_VXLAN_PORT, htobe16(v->dest_port));
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_PORT attribute: %m");

        if (v->port_range.low || v->port_range.high) {
                struct ifla_vxlan_port_range port_range;

                port_range.low = htobe16(v->port_range.low);
                port_range.high = htobe16(v->port_range.high);

                r = sd_netlink_message_append_data(m, IFLA_VXLAN_PORT_RANGE, &port_range, sizeof(port_range));
                if (r < 0)
                        return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_PORT_RANGE attribute: %m");
        }

        r = sd_netlink_message_append_u32(m, IFLA_VXLAN_LABEL, htobe32(v->flow_label));
        if (r < 0)
                return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_LABEL attribute: %m");

        if (v->group_policy) {
                r = sd_netlink_message_append_flag(m, IFLA_VXLAN_GBP);
                if (r < 0)
                        return log_netdev_error_errno(netdev, r, "Could not append IFLA_VXLAN_GBP attribute: %m");
        }

        return r;
}

int config_parse_vxlan_address(const char *unit,
                               const char *filename,
                               unsigned line,
                               const char *section,
                               unsigned section_line,
                               const char *lvalue,
                               int ltype,
                               const char *rvalue,
                               void *data,
                               void *userdata) {
        VxLan *v = userdata;
        union in_addr_union *addr = data, buffer;
        int r, f;

        assert(filename);
        assert(lvalue);
        assert(rvalue);
        assert(data);

        r = in_addr_from_string_auto(rvalue, &f, &buffer);
        if (r < 0) {
                log_syntax(unit, LOG_ERR, filename, line, r, "vxlan '%s' address is invalid, ignoring assignment: %s", lvalue, rvalue);
                return 0;
        }

        r = in_addr_is_multicast(f, &buffer);

        if (streq(lvalue, "Group")) {
                if (r <= 0) {
                        log_syntax(unit, LOG_ERR, filename, line, 0, "vxlan %s invalid multicast address, ignoring assignment: %s", lvalue, rvalue);
                        return 0;
                }

                v->remote_family = f;
        } else {
                if (r > 0) {
                        log_syntax(unit, LOG_ERR, filename, line, 0, "vxlan %s cannot be a multicast address, ignoring assignment: %s", lvalue, rvalue);
                        return 0;
                }

                if (streq(lvalue, "Remote"))
                        v->remote_family = f;
                else
                        v->local_family = f;
        }

        *addr = buffer;

        return 0;
}

int config_parse_port_range(const char *unit,
                            const char *filename,
                            unsigned line,
                            const char *section,
                            unsigned section_line,
                            const char *lvalue,
                            int ltype,
                            const char *rvalue,
                            void *data,
                            void *userdata) {
        _cleanup_free_ char *word = NULL;
        VxLan *v = userdata;
        unsigned low, high;
        int r;

        assert(filename);
        assert(lvalue);
        assert(rvalue);
        assert(data);

        r = extract_first_word(&rvalue, &word, NULL, 0);
        if (r < 0) {
                log_syntax(unit, LOG_ERR, filename, line, r, "Failed to extract VXLAN port range, ignoring: %s", rvalue);
                return 0;
        }

        if (r == 0)
                return 0;

        r = parse_range(word, &low, &high);
        if (r < 0) {
                log_syntax(unit, LOG_ERR, filename, line, r, "Failed to parse VXLAN port range '%s'", word);
                return 0;
        }

        if (low <= 0 || low > 65535 || high <= 0 || high > 65535) {
                log_syntax(unit, LOG_ERR, filename, line, r,
                           "Failed to parse VXLAN port range '%s'. Port should be greater than 0 and less than 65535.", word);
                return 0;
        }

        if (high < low) {
                log_syntax(unit, LOG_ERR, filename, line, r,
                           "Failed to parse VXLAN port range '%s'. Port range %u .. %u not valid", word, low, high);
                return 0;
        }

        v->port_range.low = low;
        v->port_range.high = high;

        return 0;
}

int config_parse_flow_label(const char *unit,
                            const char *filename,
                            unsigned line,
                            const char *section,
                            unsigned section_line,
                            const char *lvalue,
                            int ltype,
                            const char *rvalue,
                            void *data,
                            void *userdata) {
        VxLan *v = userdata;
        unsigned f;
        int r;

        assert(filename);
        assert(lvalue);
        assert(rvalue);
        assert(data);

        r = safe_atou(rvalue, &f);
        if (r < 0) {
                log_syntax(unit, LOG_ERR, filename, line, r, "Failed to parse VXLAN flow label '%s'.", rvalue);
                return 0;
        }

        if (f & ~VXLAN_FLOW_LABEL_MAX_MASK) {
                log_syntax(unit, LOG_ERR, filename, line, r,
                           "VXLAN flow label '%s' not valid. Flow label range should be [0-1048575].", rvalue);
                return 0;
        }

        v->flow_label = f;

        return 0;
}

static int netdev_vxlan_verify(NetDev *netdev, const char *filename) {
        VxLan *v = VXLAN(netdev);

        assert(netdev);
        assert(v);
        assert(filename);

        if (v->id > VXLAN_VID_MAX) {
                log_warning("VXLAN without valid Id configured in %s. Ignoring", filename);
                return -EINVAL;
        }

        return 0;
}

static void vxlan_init(NetDev *netdev) {
        VxLan *v;

        assert(netdev);

        v = VXLAN(netdev);

        assert(v);

        v->id = VXLAN_VID_MAX + 1;
        v->learning = true;
        v->udpcsum = false;
        v->udp6zerocsumtx = false;
        v->udp6zerocsumrx = false;
}

const NetDevVTable vxlan_vtable = {
        .object_size = sizeof(VxLan),
        .init = vxlan_init,
        .sections = "Match\0NetDev\0VXLAN\0",
        .fill_message_create = netdev_vxlan_fill_message_create,
        .create_type = NETDEV_CREATE_STACKED,
        .config_verify = netdev_vxlan_verify,
};
