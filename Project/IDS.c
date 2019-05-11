#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "net/routing/rpl-lite/rpl.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#include "data-sent.h"
static struct simple_udp_connection udp_conn;
//static struct data_sent da_received;
//static struct node_stats nd_stats;
//static node_stats_storage nss;

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  /* Convert from uint8_t to struct data_sent */
  const data_sent *test = (data_sent *) data;

  /* If the struct contains "alarm_*" control, proceed with stats storage */
  if (!strcmp((char *) test->control,"alarm_DIO")) {
    LOG_INFO("Received DIO alarm from ");
    LOG_INFO_6ADDR(&test->node_ipaddr);
    LOG_INFO_("\n");
  } else if (!strcmp((char *) test->control,"alarm_DIS")) {
    LOG_INFO("Received DIS alarm from ");
    LOG_INFO_6ADDR(&test->node_ipaddr);
    LOG_INFO_("\n");
  } else if (!strcmp((char *) test->control,"alarm_DAO")) {
    LOG_INFO("Received DAO alarm from ");
    LOG_INFO_6ADDR(&test->node_ipaddr);
    LOG_INFO_("\n");
  } else if (!strcmp((char *) test->control,"alarm_VNU")) { 
    LOG_INFO("Received Version number attack from ");
    LOG_INFO_6ADDR(&test->node_ipaddr);
    LOG_INFO_("\n");
  } else {
    LOG_INFO("Unable to understand message from ");
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();
  
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  /* Initialize the node_stats_storage matrix*/

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
