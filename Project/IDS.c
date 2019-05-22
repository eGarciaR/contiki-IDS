#include "contiki.h"
#include "net/routing/routing.h"

#include "net/routing/rpl-lite/rpl.h"
#include "net/ipv6/uip-icmp6.h"

#include "net/ipv6/uiplib.h" /* Just added to printf IPv6 addresses */
#include <stdio.h>

#define SEND_INTERVAL     (5 * CLOCK_SECOND)

static struct uip_icmp6_echo_reply_notification icmp_notification;
static bool heartbeat_alarm, no_reply;

PROCESS(initialize_IDS, "Initialize IDS");
PROCESS(ids_server_process, "IDS server");
PROCESS(ids_sensor_heartbeat_check, "IDS Sensor Heartbeat");
AUTOSTART_PROCESSES(&initialize_IDS, &ids_server_process, &ids_sensor_heartbeat_check);

void
icmp_reply_handler(uip_ipaddr_t *source, uint8_t ttl,
                   uint8_t *data, uint16_t datalen)
{
  uint8_t i;
  for(i=0; i < MAX_NODE_NEIGHBOR && ids_node_stats_list[i].used; ++i) {
    if (uip_ip6addr_cmp(source, &ids_node_stats_list[i].ipaddr)) {
      char buf[40];
      uiplib_ipaddr_snprint(buf, sizeof(buf), &ids_node_stats_list[i].ipaddr);
      printf("Echo response received from: %s\n", buf);
      ids_node_stats_list[i].ping_response_received = true;
      break;
    }
  }
}

static void
IDS_sensor_hearbeat()
{
  char buf[21];
  int i;
  /* Loop over all nodes */
  for(i=0; i < MAX_NODE_NEIGHBOR && ids_node_stats_list[i].used; ++i) {
    /* If not ping sent, send it and break loop to wait 5 more seconds for the next ping */
    if (!ids_node_stats_list[i].ping_message_sent)
    {
      ids_node_stats_list[i].ping_message_sent = true;
      uiplib_ipaddr_snprint(buf, sizeof(buf), &ids_node_stats_list[i].ipaddr);
      printf("Sending a new ping to: %s\n", buf);
      uip_icmp6_send(&ids_node_stats_list[i].ipaddr, ICMP6_ECHO_REQUEST, 0, 20);
      break;
    }
  }
  /* If we achieve the end of the loop, all messages have been sent. Proceed to check the reply */
  if (i>=MAX_NODE_NEIGHBOR || !ids_node_stats_list[i].used) {
    for(i=0; i < MAX_NODE_NEIGHBOR && ids_node_stats_list[i].used; ++i) {
      /* If no response, increment failures and send again */
      if (!ids_node_stats_list[i].ping_response_received) {
        uiplib_ipaddr_snprint(buf, sizeof(buf), &ids_node_stats_list[i].ipaddr);
        printf("Not ping received yet from: %s\n", buf);
        ++ids_node_stats_list[i].heartbeat_failures;
        uip_icmp6_send(&ids_node_stats_list[i].ipaddr, ICMP6_ECHO_REQUEST, 0, 20);
        no_reply = true;
        /* If failures > FAILURES_THRESHOLD, rise an alarm */
        if (ids_node_stats_list[i].heartbeat_failures > 5) {
          uiplib_ipaddr_snprint(buf, sizeof(buf), &ids_node_stats_list[i].ipaddr);
          printf("ALARM: Node %s not reachable.\n", buf);
          heartbeat_alarm = true;
          ids_node_stats_list[i].heartbeat_failures = 0;
        }
      }
    }
  }
  /* Reset everything to start Heartbeat again, either if the alarm is raised or everything is successful */
  if (heartbeat_alarm || (!no_reply && (i>=MAX_NODE_NEIGHBOR || !ids_node_stats_list[i].used))) {
    for(i=0; i < MAX_NODE_NEIGHBOR && ids_node_stats_list[i].used; ++i) {
      ids_node_stats_list[i].ping_message_sent = false;
      ids_node_stats_list[i].ping_response_received = false;
      heartbeat_alarm = false;
      no_reply = false;
    }
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(initialize_IDS, ev, data)
{
  PROCESS_BEGIN();
  
  init_IDS_server();
  
  PROCESS_END()
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ids_server_process, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();
  
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  etimer_set(&et, 60 * CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    check_stats();

    int i;
    for(i=0; i < MAX_SENSORS && ids_sensors_list[i].used; ++i) {
      char buf[40];
      uiplib_ipaddr_snprint(buf, sizeof(buf), &ids_sensors_list[i].ipaddr);
      printf("Retrieve info from: %s\n", buf);
      rpl_icmp6_ids_info_output(&ids_sensors_list[i].ipaddr);
    }

    etimer_reset(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(ids_sensor_heartbeat_check, ev, data)
{
  static struct etimer periodic_timer;
  uip_icmp6_echo_reply_callback_add(&icmp_notification, icmp_reply_handler);
  PROCESS_BEGIN();
  
  /* Initialize UDP connection */
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    IDS_sensor_hearbeat();

    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
