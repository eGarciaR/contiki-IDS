/*
 * Copyright (c) 2017, Inria.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *         Header file for rpl-ext-header
 *
 * \author Simon Duquennoy <simon.duquennoy@inria.fr>
 */

 #ifndef RPL_ICMP6_H_
 #define RPL_ICMP6_H_

#include "uip.h"
#include "uip-ds6.h"
#include "uip-ds6-nbr.h"

/* DEFINE IDS CODE */
#define MAX_DIO_THRESHOLD 50
#define MAX_DIS_THRESHOLD 20
#define MAX_DAO_THRESHOLD 40

#define MAX_HEARTBEAT_FAILURES 5

#define MAX_NODE_NEIGHBOR 16
#define MAX_SENSORS       10

/********** Data structures **********/

/* Logical representation of a DAG Information Object (DIO.) */
struct rpl_dio {
  uip_ipaddr_t dag_id;
  rpl_ocp_t ocp;
  rpl_rank_t rank;
  uint8_t grounded;
  uint8_t mop;
  uint8_t preference;
  uint8_t version;
  uint8_t instance_id;
  uint8_t dtsn;
  uint8_t dag_intdoubl;
  uint8_t dag_intmin;
  uint8_t dag_redund;
  uint8_t default_lifetime;
  uint16_t lifetime_unit;
  rpl_rank_t dag_max_rankinc;
  rpl_rank_t dag_min_hoprankinc;
  rpl_prefix_t destination_prefix;
  rpl_prefix_t prefix_info;
  struct rpl_metric_container mc;
};
typedef struct rpl_dio rpl_dio_t;

/* Logical representation of a Destination Advertisement Object (DAO.) */
struct rpl_dao {
  uip_ipaddr_t parent_addr;
  uip_ipaddr_t prefix;
  uint16_t sequence;
  uint8_t instance_id;
  uint8_t lifetime;
  uint8_t prefixlen;
  uint8_t flags;
};
typedef struct rpl_dao rpl_dao_t;

/*Additional code and variables for IDS implementation*/
/*Struct to send control data to IDS*/
typedef struct data_sent{
  char control[10];
  uip_ipaddr_t node_ipaddr;
} data_sent;

typedef struct node_counter {
  struct node_counter *next;
  uip_ipaddr_t ipaddr;
  uint8_t DIO_counter;
  uint8_t DIS_counter;
  bool DIO_version_attack;
  bool used;
} node_counter;

typedef struct ids_node_stats_counter {
  struct ids_node_stats_counter *next;
  uip_ipaddr_t ipaddr;
  uip_ipaddr_t parent;
  uint8_t DIO_counter;
  uint8_t DIS_counter;
  uint8_t DAO_counter;
  uint8_t heartbeat_failures;
  bool DIO_version_attack;
  bool used;
  bool ping_response_received;
  bool ping_message_sent;
} ids_node_stats_counter;

typedef struct ids_sensor {
  struct ids_sensor *next;
  uip_ipaddr_t ipaddr;
  bool used;
} ids_sensor; 

bool IDS_SERVER;
bool IDS_NODE_SENSOR;
bool DISCOVERY_ACK;

struct node_counter node_stats_list[MAX_NODE_NEIGHBOR];
struct ids_node_stats_counter ids_node_stats_list[MAX_NODE_NEIGHBOR];
struct ids_sensor ids_sensors_list[MAX_SENSORS];

void rpl_icmp6_node_ids_output(uip_ipaddr_t *to, int code, const void *data, uint16_t datalen);

void rpl_icmp6_ids_info_output(uip_ipaddr_t *to);

void init_IDS_server();

void init_IDS_node_sensor();

bool discovery_ack_received();

void initialize_control_messages_received();

void check_stats();

void check_node_not_reachable();

void execute_dao_insider_attack();

void set_blackhole();

/*------------------------------------------------------*/

/********** Public functions **********/

/**
 * Updates IPv6 neighbor cache on incoming link-local RPL ICMPv6 messages.
 *
 * \param from The source link-local IPv6 address
 * \param reason What triggered the update (maps to RPL packet types)
 * \param data Generic pointer, used for instance to store parsed DIO data
 * \return A pointer to the new neighbor cache entry, NULL if there was no space left.
*/
uip_ds6_nbr_t *rpl_icmp6_update_nbr_table(uip_ipaddr_t *from, nbr_table_reason_t reason, void *data);

/**
 * Creates an ICMPv6 DIS packet and sends it. Can be unicast or multicast.
 *
 * \param addr The link-local address of the target host, if any.
 * Else, a multicast DIS will be sent.
*/
void rpl_icmp6_dis_output(uip_ipaddr_t *addr);

/**
 * Creates an ICMPv6 DIO packet and sends it. Can be unicast or multicast
 *
 * \param uc_addr The link-local address of the target host, if any.
 * Else, a multicast DIO will be sent.
*/
void rpl_icmp6_dio_output(uip_ipaddr_t *uc_addr);

/**
 * Creates an ICMPv6 DAO packet and sends it to the root, advertising the
 * current preferred parent, and with our global address as prefix.
 *
 * \param lifetime The DAO lifetime. Use 0 to send a No-path DAO
*/
void rpl_icmp6_dao_output(uint8_t lifetime);

/**
 * Creates an ICMPv6 DAO-ACK packet and sends it to the originator
 * of the ACK.
 *
 * \param dest The DAO-ACK destination (was source of the DAO)
 * \param sequence The sequence number of the DAO being ACKed
 * \param status The status of the DAO-ACK (see RPL_DAO_ACK_* defines)
*/
void rpl_icmp6_dao_ack_output(uip_ipaddr_t *dest, uint8_t sequence, uint8_t status);

/**
 * Initializes rpl-icmp6 module, registers ICMPv6 handlers for all
 * RPL ICMPv6 messages: DIO, DIS, DAO and DAO-ACK
*/
void rpl_icmp6_init(void);

 /** @} */

#endif /* RPL_ICMP6_H_ */
