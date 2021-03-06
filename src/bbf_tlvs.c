/*
 *  Broadband Forum IEEE 1905.1/1a stack
 *
 *  Copyright (c) 2017, Broadband Forum
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  Subject to the terms and conditions of this license, each copyright
 *  holder and contributor hereby grants to those receiving rights under
 *  this license a perpetual, worldwide, non-exclusive, no-charge,
 *  royalty-free, irrevocable (except for failure to satisfy the
 *  conditions of this license) patent license to make, have made, use,
 *  offer to sell, sell, import, and otherwise transfer this software,
 *  where such license applies only to those patent claims, already
 *  acquired or hereafter acquired, licensable by such copyright holder or
 *  contributor that are necessarily infringed by:
 *
 *  (a) their Contribution(s) (the licensed copyrights of copyright holders
 *      and non-copyrightable additions of contributors, in source or binary
 *      form) alone; or
 *
 *  (b) combination of their Contribution(s) with the work of authorship to
 *      which such Contribution(s) was added by such copyright holder or
 *      contributor, if, at the time the Contribution is added, such addition
 *      causes such combination to be necessarily infringed. The patent
 *      license shall not apply to any other combinations which include the
 *      Contribution.
 *
 *  Except as expressly stated above, no rights or licenses from any
 *  copyright holder or contributor is granted under this license, whether
 *  expressly, by implication, estoppel or otherwise.
 *
 *  DISCLAIMER
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *  DAMAGE.
 */

#include "platform.h"

#include "bbf_tlvs.h"
#include "1905_tlvs.h"
#include "packet_tools.h"

#include <string.h> // memcmp(), memcpy(), ...
#include <stdio.h>  // snprintf

////////////////////////////////////////////////////////////////////////////////
// Actual API functions
////////////////////////////////////////////////////////////////////////////////

struct tlv *parse_bbf_TLV_from_packet(const uint8_t *packet_stream)
{
    if (NULL == packet_stream)
    {
        return NULL;
    }

    // The first byte of the stream is the "Type" field from the TLV structure.
    // Valid values for this byte are the following ones...
    //
    switch (*packet_stream)
    {
        case BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY:
        {
            // This parsing is done according to the information detailed in
            // ...
            // @todo refactor with 1905 linkMetricQuery parse function.

            struct linkMetricQueryTLV  *ret;

            const uint8_t *p;
            uint16_t len;

            uint8_t destination;
            uint8_t link_metrics_type;

            p = packet_stream + 1;
            _E2B(&p, &len);

            // The length *must* be 8
            //
            if (8 != len)
            {
               // Malformed packet
               //
               return NULL;
            }

            ret = linkMetricQueryTLVAllocAll(NULL, LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS);
            ret->tlv.type = BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY;

            _E1B(&p, &destination);
            _EnB(&p, ret->specific_neighbor, 6);

            if (destination >= 2)
            {
               // Reserved (invalid) value received
               //
               free(ret);
               return NULL;
            }
            else if (0 == destination)
            {
               uint8_t dummy_address[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

               ret->destination = LINK_METRIC_QUERY_TLV_ALL_NEIGHBORS;
               memcpy(ret->specific_neighbor, dummy_address, 6);
            }
            else if (1 == destination)
            {
               ret->destination = LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR;
            }
            else
            {
               // This code cannot be reached
               //
               free(ret);
               return NULL;
            }

            _E1B(&p, &link_metrics_type);

            if (link_metrics_type >= 3)
            {
               // Reserved (invalid) value received
               //
               free(ret);
               return NULL;
            }
            else if (0 == link_metrics_type)
            {
               ret->link_metrics_type = LINK_METRIC_QUERY_TLV_TX_LINK_METRICS_ONLY;
            }
            else if (1 == link_metrics_type)
            {
               ret->link_metrics_type = LINK_METRIC_QUERY_TLV_RX_LINK_METRICS_ONLY;
            }
            else if (2 == link_metrics_type)
            {
               ret->link_metrics_type = LINK_METRIC_QUERY_TLV_BOTH_TX_AND_RX_LINK_METRICS;
            }
            else
            {
               // This code cannot be reached
               //
               free(ret);
               return NULL;
            }

            return &ret->tlv;
        }

        case BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC:
        {
            // This parsing is done according to the information detailed in
            // ...

            struct transmitterLinkMetricTLV  *ret;

            const uint8_t *p;
            uint16_t len;
            uint8_t  i;
            uint8_t empty_address[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            ret = (struct transmitterLinkMetricTLV *)memalloc(sizeof(struct transmitterLinkMetricTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // The length *must* be "12+29*n" where "n" is "1" or greater
            //
            if ((12+29*1) > len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }
            if (0 != (len-12)%29)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC;

            _EnB(&p, ret->local_al_address,    6);
            _EnB(&p, ret->neighbor_al_address, 6);

            // Neighbor AL mac address *must* be set to zero for non-1905 devices
            //
            if (0 != memcmp(ret->neighbor_al_address, empty_address, 6))
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->transmitter_link_metrics_nr = (len-12)/29;

            ret->transmitter_link_metrics = (struct _transmitterLinkMetricEntries *)memalloc(sizeof(struct _transmitterLinkMetricEntries) * ret->transmitter_link_metrics_nr);

            for (i=0; i < ret->transmitter_link_metrics_nr; i++)
            {
                _EnB(&p,  ret->transmitter_link_metrics[i].local_interface_address,    6);
                _EnB(&p,  ret->transmitter_link_metrics[i].neighbor_interface_address, 6);

                _E2B(&p, &ret->transmitter_link_metrics[i].intf_type);
                _E1B(&p, &ret->transmitter_link_metrics[i].bridge_flag);
                _E4B(&p, &ret->transmitter_link_metrics[i].packet_errors);
                _E4B(&p, &ret->transmitter_link_metrics[i].transmitted_packets);
                _E2B(&p, &ret->transmitter_link_metrics[i].mac_throughput_capacity);
                _E2B(&p, &ret->transmitter_link_metrics[i].link_availability);
                _E2B(&p, &ret->transmitter_link_metrics[i].phy_rate);
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                free(ret->transmitter_link_metrics);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }


        case BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC:
        {
            // This parsing is done according to the information detailed in
            // ...

            struct receiverLinkMetricTLV  *ret;

            const uint8_t *p;
            uint16_t len;
            uint8_t  i;
            uint8_t empty_address[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            ret = (struct receiverLinkMetricTLV *)memalloc(sizeof(struct receiverLinkMetricTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // The length *must* be "12+23*n" where "n" is "1" or greater
            //
            if ((12+23*1) > len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }
            if (0 != (len-12)%23)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC;

            _EnB(&p, ret->local_al_address,    6);
            _EnB(&p, ret->neighbor_al_address, 6);

            // Neighbor AL mac address *must* be set to zero for non-1905 devices
            //
            if (0 != memcmp(ret->neighbor_al_address, empty_address, 6))
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->receiver_link_metrics_nr = (len-12)/23;

            ret->receiver_link_metrics = (struct _receiverLinkMetricEntries *)memalloc(sizeof(struct _receiverLinkMetricEntries) * ret->receiver_link_metrics_nr);

            for (i=0; i < ret->receiver_link_metrics_nr; i++)
            {
                _EnB(&p,  ret->receiver_link_metrics[i].local_interface_address,    6);
                _EnB(&p,  ret->receiver_link_metrics[i].neighbor_interface_address, 6);

                _E2B(&p, &ret->receiver_link_metrics[i].intf_type);
                _E4B(&p, &ret->receiver_link_metrics[i].packet_errors);
                _E4B(&p, &ret->receiver_link_metrics[i].packets_received);
                _E1B(&p, &ret->receiver_link_metrics[i].rssi);
            }

            if (p - (packet_stream+3) != len)
            {
                // Malformed packet
                //
                free(ret->receiver_link_metrics);
                free(ret);
                return NULL;
            }

            return &ret->tlv;
        }

        case BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE:
        {
            // This parsing is done according to the information detailed in
            // "IEEE Std 1905.1-2013 Section 6.4.13"

            struct linkMetricResultCodeTLV  *ret;

            const uint8_t *p;
            uint16_t len;

            ret = (struct linkMetricResultCodeTLV *)memalloc(sizeof(struct linkMetricResultCodeTLV));

            p = packet_stream + 1;
            _E2B(&p, &len);

            // The length *must* be 1
            //
            if (1 != len)
            {
                // Malformed packet
                //
                free(ret);
                return NULL;
            }

            ret->tlv.type = BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE;

            _E1B(&p, &ret->result_code);

            return &ret->tlv;
        }

        default:
        {
            // Ignore
            //
            return NULL;
        }

    }

    // This code cannot be reached
    //
    return NULL;
}


uint8_t *forge_bbf_TLV_from_structure(struct tlv *memory_structure, uint16_t *len)
{
    if (NULL == memory_structure)
    {
        return NULL;
    }

    // The first byte of any of the valid structures is always the "tlv_type"
    // field.
    //
    switch (memory_structure->type)
    {
        case BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY:
        {
            // This forging is done according to the information detailed in
            // ...

            uint8_t *ret, *p;
            struct linkMetricQueryTLV *m;

            uint16_t tlv_length;

            m = (struct linkMetricQueryTLV *)memory_structure;

            tlv_length = 8;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,          &p);
            _I2B(&tlv_length,           &p);
            _I1B(&m->destination,       &p);

            if (LINK_METRIC_QUERY_TLV_SPECIFIC_NEIGHBOR == m->destination)
            {
                _InB(m->specific_neighbor,  &p, 6);
            }
            else
            {
                uint8_t empty_address[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

                _InB(empty_address,  &p, 6);
            }

            _I1B(&m->link_metrics_type, &p);

            return ret;
        }

        case BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC:
        {
            // This forging is done according to the information detailed in
            // ...

            uint8_t *ret, *p;
            struct transmitterLinkMetricTLV *m;

            uint16_t tlv_length;

            uint8_t i;
            uint8_t empty_address[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            m = (struct transmitterLinkMetricTLV *)memory_structure;

            tlv_length = 12 + 29*m->transmitter_link_metrics_nr;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,            &p);
            _I2B(&tlv_length,             &p);
            _InB( m->local_al_address,    &p, 6);
            _InB( empty_address,          &p, 6);

            for (i=0; i<m->transmitter_link_metrics_nr; i++)
            {
                _InB( m->transmitter_link_metrics[i].local_interface_address,    &p, 6);
                _InB( m->transmitter_link_metrics[i].neighbor_interface_address, &p, 6);
                _I2B(&m->transmitter_link_metrics[i].intf_type,                  &p);
                _I1B(&m->transmitter_link_metrics[i].bridge_flag,                &p);
                _I4B(&m->transmitter_link_metrics[i].packet_errors,              &p);
                _I4B(&m->transmitter_link_metrics[i].transmitted_packets,        &p);
                _I2B(&m->transmitter_link_metrics[i].mac_throughput_capacity,    &p);
                _I2B(&m->transmitter_link_metrics[i].link_availability,          &p);
                _I2B(&m->transmitter_link_metrics[i].phy_rate,                   &p);
            }

            return ret;
        }

        case BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC:
        {
            // This forging is done according to the information detailed in
            // ...

            uint8_t *ret, *p;
            struct receiverLinkMetricTLV *m;

            uint16_t tlv_length;

            uint8_t i;
            uint8_t empty_address[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

            m = (struct receiverLinkMetricTLV *)memory_structure;

            tlv_length = 12 + 23*m->receiver_link_metrics_nr;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,            &p);
            _I2B(&tlv_length,             &p);
            _InB( m->local_al_address,    &p, 6);
            _InB( empty_address,          &p, 6);

            for (i=0; i<m->receiver_link_metrics_nr; i++)
            {
                _InB( m->receiver_link_metrics[i].local_interface_address,    &p, 6);
                _InB( m->receiver_link_metrics[i].neighbor_interface_address, &p, 6);
                _I2B(&m->receiver_link_metrics[i].intf_type,                  &p);
                _I4B(&m->receiver_link_metrics[i].packet_errors,              &p);
                _I4B(&m->receiver_link_metrics[i].packets_received,           &p);
                _I1B(&m->receiver_link_metrics[i].rssi,                       &p);
            }

            return ret;
        }

        case BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE:
        {
            // This forging is done according to the information detailed in
            // ...

            uint8_t *ret, *p;
            struct linkMetricResultCodeTLV *m;

            uint16_t tlv_length;

            m = (struct linkMetricResultCodeTLV *)memory_structure;

            tlv_length = 1;
            *len = 1 + 2 + tlv_length;

            p = ret = (uint8_t *)memalloc(1 + 2  + tlv_length);

            _I1B(&m->tlv.type,     &p);
            _I2B(&tlv_length,      &p);

            if (m->result_code != LINK_METRIC_RESULT_CODE_TLV_INVALID_NEIGHBOR)
            {
                // Malformed structure
                //
                free(ret);
                return NULL;
            }

            _I1B(&m->result_code,  &p);

            return ret;
        }

        default:
        {
            // Ignore
            //
            return NULL;
        }

    }

    // This code cannot be reached
    //
    return NULL;
}


void free_bbf_TLV_structure(struct tlv *memory_structure)
{
    if (NULL == memory_structure)
    {
        return;
    }

    // The first byte of any of the valid structures is always the "tlv_type"
    // field.
    //
    switch (memory_structure->type)
    {
        case BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY:
        case BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE:
        {
            free(memory_structure);

            return;
        }

        case BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC:
        {
            struct transmitterLinkMetricTLV *m;

            m = (struct transmitterLinkMetricTLV *)memory_structure;

            if (m->transmitter_link_metrics_nr > 0 && NULL != m->transmitter_link_metrics)
            {
                free(m->transmitter_link_metrics);
            }
            free(m);

            return;
        }

        case BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC:
        {
            struct receiverLinkMetricTLV *m;

            m = (struct receiverLinkMetricTLV *)memory_structure;

            if (m->receiver_link_metrics_nr > 0 && NULL != m->receiver_link_metrics)
            {
                free(m->receiver_link_metrics);
            }
            free(m);

            return;
        }

        default:
        {
            // Ignore
            //
          return;
        }
    }

    // This code cannot be reached
    //
    return;
}


uint8_t compare_bbf_TLV_structures(struct tlv *memory_structure_1, struct tlv *memory_structure_2)
{
    if (NULL == memory_structure_1 || NULL == memory_structure_2)
    {
        return 1;
    }

    // The first byte of any of the valid structures is always the "tlv_type"
    // field.
    //
    if (memory_structure_1->type != memory_structure_2->type)
    {
        return 1;
    }

    switch (memory_structure_1->type)
    {
        case BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY:
        {
            struct linkMetricQueryTLV *p1, *p2;

            p1 = (struct linkMetricQueryTLV *)memory_structure_1;
            p2 = (struct linkMetricQueryTLV *)memory_structure_2;

            if (
                                 p1->destination        !=  p2->destination                ||
                 memcmp(p1->specific_neighbor,     p2->specific_neighbor, 6) !=0  ||
                                 p1->link_metrics_type  !=  p2->link_metrics_type
               )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        case BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC:
        {
            struct transmitterLinkMetricTLV *p1, *p2;
            uint8_t i;

            p1 = (struct transmitterLinkMetricTLV *)memory_structure_1;
            p2 = (struct transmitterLinkMetricTLV *)memory_structure_2;

            if (
                 memcmp(p1->local_al_address,              p2->local_al_address,    6) !=0  ||
                 memcmp(p1->neighbor_al_address,           p2->neighbor_al_address, 6) !=0  ||
                                 p1->transmitter_link_metrics_nr != p2->transmitter_link_metrics_nr
               )
            {
                return 1;
            }

            if (p1->transmitter_link_metrics_nr > 0 && (NULL == p1->transmitter_link_metrics || NULL == p2->transmitter_link_metrics))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->transmitter_link_metrics_nr; i++)
            {
                if (
                     memcmp(p1->transmitter_link_metrics[i].local_interface_address,       p2->transmitter_link_metrics[i].local_interface_address,    6) !=0  ||
                     memcmp(p1->transmitter_link_metrics[i].neighbor_interface_address,    p2->transmitter_link_metrics[i].neighbor_interface_address, 6) !=0  ||
                                     p1->transmitter_link_metrics[i].intf_type                  !=  p2->transmitter_link_metrics[i].intf_type                           ||
                                     p1->transmitter_link_metrics[i].bridge_flag                !=  p2->transmitter_link_metrics[i].bridge_flag                         ||
                                     p1->transmitter_link_metrics[i].packet_errors              !=  p2->transmitter_link_metrics[i].packet_errors                       ||
                                     p1->transmitter_link_metrics[i].transmitted_packets        !=  p2->transmitter_link_metrics[i].transmitted_packets                 ||
                                     p1->transmitter_link_metrics[i].mac_throughput_capacity    !=  p2->transmitter_link_metrics[i].mac_throughput_capacity             ||
                                     p1->transmitter_link_metrics[i].link_availability          !=  p2->transmitter_link_metrics[i].link_availability                   ||
                                     p1->transmitter_link_metrics[i].phy_rate                   !=  p2->transmitter_link_metrics[i].phy_rate
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC:
        {
            struct receiverLinkMetricTLV *p1, *p2;
            uint8_t i;

            p1 = (struct receiverLinkMetricTLV *)memory_structure_1;
            p2 = (struct receiverLinkMetricTLV *)memory_structure_2;

            if (
                 memcmp(p1->local_al_address,           p2->local_al_address,    6) !=0  ||
                 memcmp(p1->neighbor_al_address,        p2->neighbor_al_address, 6) !=0  ||
                                 p1->receiver_link_metrics_nr != p2->receiver_link_metrics_nr
               )
            {
                return 1;
            }

            if (p1->receiver_link_metrics_nr > 0 && (NULL == p1->receiver_link_metrics || NULL == p2->receiver_link_metrics))
            {
                // Malformed structure
                //
                return 1;
            }

            for (i=0; i<p1->receiver_link_metrics_nr; i++)
            {
                if (
                     memcmp(p1->receiver_link_metrics[i].local_interface_address,       p2->receiver_link_metrics[i].local_interface_address,    6) !=0  ||
                     memcmp(p1->receiver_link_metrics[i].neighbor_interface_address,    p2->receiver_link_metrics[i].neighbor_interface_address, 6) !=0  ||
                                     p1->receiver_link_metrics[i].intf_type                  !=  p2->receiver_link_metrics[i].intf_type                           ||
                                     p1->receiver_link_metrics[i].packet_errors              !=  p2->receiver_link_metrics[i].packet_errors                       ||
                                     p1->receiver_link_metrics[i].packets_received           !=  p2->receiver_link_metrics[i].packets_received                    ||
                                     p1->receiver_link_metrics[i].rssi                       !=  p2->receiver_link_metrics[i].rssi
                   )
                {
                    return 1;
                }
            }

            return 0;
        }

        case BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE:
        {
            struct linkMetricResultCodeTLV *p1, *p2;

            p1 = (struct linkMetricResultCodeTLV *)memory_structure_1;
            p2 = (struct linkMetricResultCodeTLV *)memory_structure_2;

            if (
                 p1->result_code != p2->result_code
               )
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        default:
        {
            // Unknown structure type
            //
            return 1;
        }
    }

    // This code cannot be reached
    //
    return 1;
}


void visit_bbf_TLV_structure(struct tlv *memory_structure, visitor_callback callback, void (*write_function)(const char *fmt, ...), const char *prefix)
{
    // Buffer size to store a prefix string that will be used to show each
    // element of a structure on screen
    //
    #define MAX_PREFIX  100

    if (NULL == memory_structure)
    {
        return;
    }

    // The first byte of any of the valid structures is always the "tlv_type"
    // field.
    //
    switch (memory_structure->type)
    {
        case BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY:
        {
            struct linkMetricQueryTLV *p;

            p = (struct linkMetricQueryTLV *)memory_structure;

            callback(write_function, prefix, sizeof(p->destination),       "destination",        "%d",      &p->destination);
            callback(write_function, prefix, sizeof(p->specific_neighbor), "specific_neighbor",  "0x%02x",   p->specific_neighbor);
            callback(write_function, prefix, sizeof(p->link_metrics_type), "link_metrics_type",  "%d",      &p->link_metrics_type);

            break;
        }

        case BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC:
        {
            struct transmitterLinkMetricTLV *p;
            uint8_t i;

            p = (struct transmitterLinkMetricTLV *)memory_structure;

            if (NULL == p->transmitter_link_metrics)
            {
                // Malformed structure
                return;
            }

            callback(write_function, prefix, sizeof(p->local_al_address),            "local_al_address",            "0x%02x",   p->local_al_address);
            callback(write_function, prefix, sizeof(p->neighbor_al_address),         "neighbor_al_address",         "0x%02x",   p->neighbor_al_address);
            callback(write_function, prefix, sizeof(p->transmitter_link_metrics_nr), "transmitter_link_metrics_nr", "%d",      &p->transmitter_link_metrics_nr);
            for (i=0; i < p->transmitter_link_metrics_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%stransmitter_link_metrics[%d]->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].local_interface_address),    "local_interface_address",    "0x%02x",   p->transmitter_link_metrics[i].local_interface_address);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].neighbor_interface_address), "neighbor_interface_address", "0x%02x",   p->transmitter_link_metrics[i].neighbor_interface_address);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].intf_type),                  "intf_type",                  "0x%04x",  &p->transmitter_link_metrics[i].intf_type);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].bridge_flag),                "bridge_flag",                "%d",      &p->transmitter_link_metrics[i].bridge_flag);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].packet_errors),              "packet_errors",              "%d",      &p->transmitter_link_metrics[i].packet_errors);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].transmitted_packets),        "transmitted_packets",        "%d",      &p->transmitter_link_metrics[i].transmitted_packets);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].mac_throughput_capacity),    "mac_throughput_capacity",    "%d",      &p->transmitter_link_metrics[i].mac_throughput_capacity);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].link_availability),          "link_availability",          "%d",      &p->transmitter_link_metrics[i].link_availability);
                callback(write_function, new_prefix, sizeof(p->transmitter_link_metrics[i].phy_rate),                   "phy_rate",                   "%d",      &p->transmitter_link_metrics[i].phy_rate);
            }

            break;
        }


        case BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC:
        {
            struct receiverLinkMetricTLV *p;
            uint8_t i;

            p = (struct receiverLinkMetricTLV *)memory_structure;

            if (NULL == p->receiver_link_metrics)
            {
                // Malformed structure
                return;
            }

            callback(write_function, prefix, sizeof(p->local_al_address),         "local_al_address",         "0x%02x",   p->local_al_address);
            callback(write_function, prefix, sizeof(p->neighbor_al_address),      "neighbor_al_address",      "0x%02x",   p->neighbor_al_address);
            callback(write_function, prefix, sizeof(p->receiver_link_metrics_nr), "receiver_link_metrics_nr", "%d",      &p->receiver_link_metrics_nr);
            for (i=0; i < p->receiver_link_metrics_nr; i++)
            {
                char new_prefix[MAX_PREFIX];

                snprintf(new_prefix, MAX_PREFIX-1, "%sreceiver_link_metrics[%d]->", prefix, i);
                new_prefix[MAX_PREFIX-1] = 0x0;

                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].local_interface_address),    "local_interface_address",    "0x%02x",   p->receiver_link_metrics[i].local_interface_address);
                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].neighbor_interface_address), "neighbor_interface_address", "0x%02x",   p->receiver_link_metrics[i].neighbor_interface_address);
                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].intf_type),                  "intf_type",                  "0x%04x",  &p->receiver_link_metrics[i].intf_type);
                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].packet_errors),              "packet_errors",              "%d",      &p->receiver_link_metrics[i].packet_errors);
                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].packets_received),           "packets_received",           "%d",      &p->receiver_link_metrics[i].packets_received);
                callback(write_function, new_prefix, sizeof(p->receiver_link_metrics[i].rssi),                       "rssi",                       "%d",      &p->receiver_link_metrics[i].rssi);
            }

            break;
        }

        case BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE:
        {
            struct linkMetricResultCodeTLV *p;

            p = (struct linkMetricResultCodeTLV *)memory_structure;

            callback(write_function, prefix, sizeof(p->result_code), "result_code",  "%d",  &p->result_code);

            break;
        }

        default:
        {
            // Ignore
            //
            break;
        }
    }

    return;
}


char *convert_bbf_TLV_type_to_string(uint8_t tlv_type)
{
    switch (tlv_type)
    {
      case BBF_TLV_TYPE_NON_1905_LINK_METRIC_QUERY:
          return "TLV_TYPE_NON_1905_LINK_METRIC_QUERY";
      case BBF_TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC:
          return "TLV_TYPE_NON_1905_TRANSMITTER_LINK_METRIC";
      case BBF_TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC:
          return "TLV_TYPE_NON_1905_RECEIVER_LINK_METRIC";
      case BBF_TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE:
          return "TLV_TYPE_NON_1905_LINK_METRIC_RESULT_CODE";
      default:
          return "Unknown";
    }
}
