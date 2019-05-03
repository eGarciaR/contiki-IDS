#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/routing/rpl-lite/rpl.h"

#include "sys/log.h"

#include "data-sent.h"

#define LOG_MODULE "Node"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (20 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
static struct data_sent da_sent;

/*---------------------------------------------------------------------------*/
PROCESS(initialize_IDS, "Initialize IDS");
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&initialize_IDS, &udp_client_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(initialize_IDS, ev, data)
{
  PROCESS_BEGIN();

  initialize_control_messages_received();

  PROCESS_END()
}

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count;
  uip_ipaddr_t dest_ipaddr;
  
  strcpy(da_sent.control,"stats"); 

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, NULL);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      
      int *cmc;
      cmc = get_control_messages_count();
      da_sent.DIO_messages_received = *(cmc+0);
      da_sent.DIS_messages_received = *(cmc+1);
      da_sent.DAO_messages_received = *(cmc+2);  
      simple_udp_sendto(&udp_conn, &da_sent, sizeof(da_sent), &dest_ipaddr);
      count++;
    } else {
      LOG_INFO("Not reachable yet\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
