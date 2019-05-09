#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/routing/rpl-lite/rpl.h"

#include "sys/log.h"

#include "data-sent.h"

#include "sys/energest.h"

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
PROCESS(energest_process, "Init monitoring");
AUTOSTART_PROCESSES(&initialize_IDS, &udp_client_process, &energest_process);
/*---------------------------------------------------------------------------*/

static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

PROCESS_THREAD(initialize_IDS, ev, data)
{
  PROCESS_BEGIN();
  
  init_IDS_node_sensor();
  initialize_control_messages_received();
  
  PROCESS_END()
}

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t dest_ipaddr;
  struct node_counter *nc; 
  PROCESS_BEGIN();
  
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, NULL);
  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    bool alarm = false;
    for(nc = list_head(node_stats_list); nc != NULL; nc = list_item_next(nc)) {
      LOG_INFO("DIO: %d, DIS: %d, DIO version: %d \n",nc->DIO_counter, nc->DIS_counter, nc->DIO_version_increment_counter);
      /* HELLO FLOOD Attack */
      if (nc->DIO_counter > MAX_DIO_THRESHOLD) {
        alarm = true;
	strcpy(da_sent.control,"alarm_DIO");
	LOG_INFO("ALARM DIO: '%d'\n", nc->DIO_counter);
      } else if (nc->DIS_counter > MAX_DIS_THRESHOLD) {
        alarm = true;
        strcpy(da_sent.control,"alarm_DIS");
        LOG_INFO("ALARM DIS: '%d'\n", nc->DIS_counter);
      } else if (nc->DAO_counter > MAX_DAO_THRESHOLD) {
        alarm = true;
        strcpy(da_sent.control,"alarm_DAO");
        LOG_INFO("ALARM DAO: '%d'\n", nc->DAO_counter);
      } else if (nc->DIO_version_increment_counter > MAX_DIO_VERSION_INCREMENT) {
	/* Version number Attack */
	alarm = true;
	strcpy(da_sent.control,"alarm_VNU");
	LOG_INFO("ALARM Version number: '%d'\n", nc->DIO_version_increment_counter);
      }
      
      if (alarm) {
        uip_ipaddr_copy(&da_sent.node_ipaddr, &nc->ipaddr);
	break;
      }
    }
    if (alarm) {
      if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
        /* Send to DAG root */
	simple_udp_sendto(&udp_conn, &da_sent, sizeof(da_sent), &dest_ipaddr);
        LOG_INFO("Alarm sent\n");
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

PROCESS_THREAD(energest_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&periodic_timer, CLOCK_SECOND * 10);

  while(1) {
    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

    energest_flush();

    printf("\nEnergest:\n");
    printf(" CPU          %4lus LPM      %4lus DEEP LPM %4lus  Total time %lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
    printf(" Radio LISTEN %4lus TRANSMIT %4lus OFF      %4lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
- energest_type_time(ENERGEST_TYPE_LISTEN)));

  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
