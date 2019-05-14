#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/routing/rpl-lite/rpl.h"

#include "sys/log.h"

#include "data-sent.h"

#include "sys/energest.h"

#include "net/ipv6/uiplib.h" /* Just added to printf IPv6 addresses */

#define LOG_MODULE "Node"
#define LOG_LEVEL LOG_LEVEL_INFO

#define DISCOVERY_SEND_INTERVAL (20 * CLOCK_SECOND)
#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

static struct data_sent da_sent;

/*---------------------------------------------------------------------------*/
PROCESS(initialize_IDS_node, "Initialize IDS Node");
PROCESS(ids_node_client_process, "IDS Node client");
PROCESS(energest_process, "Init monitoring");
AUTOSTART_PROCESSES(&initialize_IDS_node/*, &ids_node_client_process*//*, &energest_process*/);
/*---------------------------------------------------------------------------*/

static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

PROCESS_THREAD(initialize_IDS_node, ev, data)
{
  static struct etimer periodic_timer;
  PROCESS_BEGIN();
  
  init_IDS_node_sensor();
  initialize_control_messages_received();
  etimer_set(&periodic_timer, random_rand() % DISCOVERY_SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    if (!discovery_ack_received()) {
      // SEND MESSAGE AGAIN
      uip_ipaddr_t ip_root_cpy;
      if (rpl_dag_get_root_ipaddr(&ip_root_cpy)) {
        char message[10];
        strcpy(message,"DISCOVERY");
        rpl_icmp6_node_ids_output(&ip_root_cpy, 0, &message, sizeof(message));
        LOG_INFO("Discovery sent\n");
      } else {
        printf("ERROR: Instance has no root\n");
      }
    } else {
      printf("IDS Ack received\n");
      process_start(&ids_node_client_process, NULL);
      break;
    }
    /* Add some jitter */
    etimer_set(&periodic_timer, DISCOVERY_SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }
  
  PROCESS_END()
}

PROCESS_THREAD(ids_node_client_process, ev, data)
{
  static struct etimer periodic_timer;
  struct node_counter *nc; 
  PROCESS_BEGIN();
  
  /* Initialize UDP connection */
  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    bool alarm = false;
    for(nc = list_head(node_stats_list); nc != NULL; nc = list_item_next(nc)) {
      char buf[40];
      uiplib_ipaddr_snprint(buf, sizeof(buf), &nc->ipaddr);
      LOG_INFO("IP: %s,DIO: %d, DIS: %d, DIO version attack?: %s \n",buf, nc->DIO_counter, nc->DIS_counter, nc->DIO_version_attack ? "true" : "false");
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
      } else if (nc->DIO_version_attack) {
	      /* Version number Attack */
	      alarm = true;
	     strcpy(da_sent.control,"alarm_VNU");
	     LOG_INFO("ALARM Version number attack received from: ");
	     LOG_INFO_6ADDR(&nc->ipaddr);
       LOG_INFO_("\n");
      }
      
      if (alarm) {
        uip_ipaddr_copy(&da_sent.node_ipaddr, &nc->ipaddr);
	      break;
      }
    }
    LOG_INFO("---------------------------------------------------------------\n");
    if (alarm) {
      // TODO: remove this. Do it from the first process. 
      uip_ipaddr_t ip_root_cpy;
      if (rpl_dag_get_root_ipaddr(&ip_root_cpy)) {
        rpl_icmp6_node_ids_output(&ip_root_cpy, 1, &da_sent, sizeof(da_sent));
        LOG_INFO("Alarm sent\n");
        initialize_control_messages_received();
      } else {
        printf("ERROR: Instance has no root\n");
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
