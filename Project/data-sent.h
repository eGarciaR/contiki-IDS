//This library represents the data structure to be sent from the nodes to the IDS
/*---------------------------------------------------------------------------*/
#ifndef DATA_SENT_H_
#define DATA_SENT_H_
/*---------------------------------------------------------------------------*/
typedef struct data_sent{
	char control[5];
        int DIO_messages_received;
        int DIS_messages_received;
	int DAO_messages_received;
} data_sent;
/*---------------------------------------------------------------------------*/
#endif /* DATA_SENT_H_ */
/*---------------------------------------------------------------------------*/
