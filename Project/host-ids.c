#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/ipv6/uip-icmp6.h"

#include "data-sent.h"

#include "sys/energest.h"

#include "net/ipv6/uiplib.h" /* Just added to printf IPv6 addresses */
#include <stdio.h>

#define DISCOVERY_SEND_INTERVAL (20 * CLOCK_SECOND)
#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(initialize_IDS_sensor, "Initialize IDS sensor");
PROCESS(ids_sensor_heartbeat_check, "IDS Sensor Heartbeat");
PROCESS(energest_process, "Init monitoring");
AUTOSTART_PROCESSES(&initialize_IDS_sensor /*&ids_sensor_heartbeat_check*//*, &energest_process*/);
/*---------------------------------------------------------------------------*/

static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

PROCESS_THREAD(initialize_IDS_sensor, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();
  
  init_IDS_node_sensor();
  initialize_control_messages_received();
  etimer_set(&et, DISCOVERY_SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    if (!discovery_ack_received()) {
      // SEND MESSAGE AGAIN
      uip_ipaddr_t ip_root_cpy;
      if (rpl_dag_get_root_ipaddr(&ip_root_cpy)) {
        char message[10];
        strcpy(message,"DISCOVERY");
        rpl_icmp6_node_ids_output(&ip_root_cpy, 0, &message, sizeof(message));
        printf("Discovery sent\n");
      } else {
        printf("ERROR: Instance has no root\n");
      }
    } else {
      printf("IDS Sensor started...\n");
      //process_start(&ids_node_client_process, NULL);
      break;
    }
    etimer_reset(&et);
  }
  
  PROCESS_END()
}

PROCESS_THREAD(ids_sensor_heartbeat_check, ev, data)
{
  static struct etimer periodic_timer;
  PROCESS_BEGIN();
  
  /* Initialize UDP connection */
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    etimer_reset(&periodic_timer);
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
