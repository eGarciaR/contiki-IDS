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

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

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
  
  set_node_sensor();
  initialize_control_messages_received();
  
  PROCESS_END()
}

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t dest_ipaddr;
  
  PROCESS_BEGIN();
  
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, NULL);
  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    bool alarm = false;
    int8_t *cmc;
    cmc = get_control_messages_count();
    LOG_INFO("'%d', '%d', '%d'\n", *(cmc+0), *(cmc+1), *(cmc+2));
    if (*(cmc+0) > MAX_DIO_THRESHOLD) {
      alarm = true;
      strcpy(da_sent.control,"alarm_DIO");
      LOG_INFO("ALARM DIO: '%d'\n", *(cmc+0));
    } else if (*(cmc+1) > MAX_DIS_THRESHOLD) {
      alarm = true;
      strcpy(da_sent.control,"alarm_DIS");
      LOG_INFO("ALARM DIS: '%d'\n", *(cmc+1));
    } else if (*(cmc+2) > MAX_DAO_THRESHOLD) {
      alarm = true;
      strcpy(da_sent.control,"alarm_DAO");
      LOG_INFO("ALARM DAO: '%d'\n", *(cmc+2));
    }

    if (alarm) {
      if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
        /* Send to DAG root */
	simple_udp_sendto(&udp_conn, da_sent.control, sizeof(da_sent.control), &dest_ipaddr);
        LOG_INFO("Alarm sent");
	initialize_control_messages_received();
      } else {
	LOG_INFO("Not reachable yet\n");
      }
    } else {
      initialize_control_messages_received();
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
