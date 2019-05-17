//This library represents the data structure to be sent from the nodes to the IDS
/*---------------------------------------------------------------------------*/
#ifndef DATA_SENT_H_
#define DATA_SENT_H_
/*---------------------------------------------------------------------------*/

//#include "net/ipv6/uip.h"

#define MAX_NODE_STORAGE 10 
#define MAX_NODE_STATS 10
#define MAX_DIO_THRESHOLD 50
#define MAX_DIS_THRESHOLD 20

/*Struct to send control data to IDS*/
/*typedef struct data_sent{
  char control[10];
  uip_ipaddr_t node_ipaddr;
} data_sent;*/

/*Struct to store node stats*/
typedef struct node_stats{
  uip_ipaddr_t node_ipaddr;
  int DIO_messages_received;
  int DIS_messages_received;
  int DAO_messages_received;
} node_stats;

/*Matrix of node_stats to store all the stats from the nodes*/
typedef node_stats node_stats_storage[MAX_NODE_STORAGE][MAX_NODE_STATS];

/*---------------------------------------------------------------------------*/
#endif /* DATA_SENT_H_ */
/*---------------------------------------------------------------------------*/
