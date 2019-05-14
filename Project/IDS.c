#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "net/routing/rpl-lite/rpl.h"

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
  PROCESS_BEGIN();
  
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
