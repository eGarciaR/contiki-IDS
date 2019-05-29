#include "contiki.h"
#include <stdio.h> /* For printf() */

#include "net/routing/rpl-lite/rpl.h"

#include "sys/energest.h"

/*---------------------------------------------------------------------------*/
PROCESS(dummy_process, "Dummy process");
PROCESS(dao_insider_attack, "DAO Insider Attack");
AUTOSTART_PROCESSES(&dummy_process/*, &dao_insider_attack*/);
/*---------------------------------------------------------------------------*/

static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

PROCESS_THREAD(dummy_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&periodic_timer, CLOCK_SECOND * 240);

  while(1) {
    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

    //set_blackhole();

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

PROCESS_THREAD(dao_insider_attack, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&periodic_timer, CLOCK_SECOND);

  while(1) {
    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    execute_dao_insider_attack();

    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
