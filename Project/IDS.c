#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "net/routing/rpl-lite/rpl.h"

#include "net/ipv6/uiplib.h" /* Just added to printf IPv6 addresses */

#include "sys/log.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(initialize_IDS, "Initialize IDS");
PROCESS(ids_server_process, "IDS server");
AUTOSTART_PROCESSES(&initialize_IDS, &ids_server_process);

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
    for(i=0; i < 10 && ids_sensors_list[i].used; ++i) {
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
