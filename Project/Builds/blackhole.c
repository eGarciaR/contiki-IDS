#include "contiki.h"
#include <stdio.h> /* For printf() */

#include "net/routing/rpl-lite/rpl.h"

/*---------------------------------------------------------------------------*/
PROCESS(dummy_process, "Dummy process");
AUTOSTART_PROCESSES(&dummy_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(dummy_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&periodic_timer, CLOCK_SECOND * 240);

  while(1) {
    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    set_blackhole();

    //etimer_reset(&periodic_timer);

  
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
