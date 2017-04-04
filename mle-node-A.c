/*
 * mle-node-A.c
 *
 *  Created on: Mar 20, 2017
 *      Author: user
 */

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "net/ipv6/mle/mle-commands.h"
#include "sys/node-id.h"

#include "simple-udp.h"
#include "servreg-hack.h"

#include <stdio.h>
#include <string.h>



#define UDP_PORT 1234					//unicast-sender uses 1234
#define SERVICE_ID 190

#define SEND_INTERVAL		(60 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))			//Double check what this time should be

#define RECEIVE_INTERVAL		(60 * CLOCK_SECOND)

static struct simple_udp_connection unicast_connection;				//Check if this needs to be adapted for MLE


uint8_t outputbuf[20] = {0};				//Maybe change this with macro for max length
uint16_t outputbuf_length;					//Why did I decide on 16 bit?
uint8_t flag =0;
PROCESS(mle_node_A_send_process,"mle link request example");
AUTOSTART_PROCESSES(&mle_node_A_send_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("Data received on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);
}
/*------------------------------------------------------------------------------------------------------------------------------------------------------*/
void data_received_sender(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
	//may want to call security header check function here

	//ignore messages with IP hop limit that is not 255
	//ignore messages whose Command Type is reserved ( 7 - 255)
	//ignore messages not secured with link-layer security or MLE security
		//Unless it is a node just joining the network and does not have the correct "keys" yet
	//Secured incoming messages are decrypted and authenticated using procedures specified in [AES] and [CCM]
		//With security material obtained from the aux header

//*********************************Link Reject Space***************************************************************/
	//If too many Link Requests arrive
	//If a message is receives a secured unicast message for which it does not have link config data
		//If a large number of such messages arrive a device may send a Link Reject
	//May want to consider placing a Link Reject option in all Handler functions
//******************************************************************************************************************/
	printf("DEBUGGING: inside data_receiver_sender");
	uint8_t *ptr_flag;
	ptr_flag = &flag;
	 //Maybe this could go in a new function called command_reader?
	 switch(data[1]){

	 	 case 0:							//Link Request
	 		//outputbuf_length = link_request_in(data,datalen,outputbuf);
	 		outputbuf_length = link_request_in(data,datalen,outputbuf);
	 		 break;

	 	 case 1:							//Link Accept
	 		//outputbuf_length = link_accept_in(data,datalen,outputbuf);
	 		 break;
	 	 case 2:							//Link Reject
	 		 break;
	 	 case 3:							//Link Accept and Request
	 		 	outputbuf_length = link_accept_and_request_in(data,datalen,outputbuf);
				*ptr_flag =1;
				printf("\n\nflag is equal to %u",flag);
				//simple_udp_send_to(c, outputbuf, outputbuf_length, unicast_connection.remote_addr);
	 		 break;
	 	 case 4:							//Advertisement
	 	//	outputbuf_length = advertisement_in(data,datalen);
	 		 break;
	 	 case 5:							//Update
	 		 break;
	 	 case 6:							//Update request
	 		 break;

	 }
	    printf("\nSent data is: ");
		uint8_t i;
		for(i=0;i<outputbuf_length;i++){

		printf("%u  ",outputbuf[i]);

		}
		printf("\n");
	 //return outputbuf_length;
}
/*static uip_ipaddr_t *
set_global_address(void)																//Taken from unicast-receiver.c
{
  static uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);					//Sets the IPv6 address
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);											//Applies link-local(?) address to IPv6 and toggle link-local/multicast bit
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  printf("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }

  return &ipaddr;																		//RETURNS ITS OWN IP ADDRESS
}
*/
/*------------------------------------------------------------------------------*/
static void
set_global_address(void)																//Original from unicast-sender.c
{
  uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  printf("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}
/*------------------------------------------------------------------------------*/
PROCESS_THREAD(mle_node_A_send_process, ev, data)
{

	static struct etimer periodic_timer;
	static struct etimer send_timer;
	static struct etimer receive_timer;

	uint8_t i;

	uip_ipaddr_t *addr;
	uip_ipaddr_t *ipaddr;																//Store the source IP


	PROCESS_BEGIN();


    servreg_hack_init();

    set_global_address();																//This has been removed, should i put it back?: will it conflict with SourceAddress TLV


    simple_udp_register(&unicast_connection, UDP_PORT,
                        NULL, UDP_PORT, receiver);

    //Port will change as not expecting data and no remote-address yet
    printf("DEBUGGING: remote port is %u\n",&unicast_connection.local_port);
    printf("DEBUGGING: remote port is %u\n",&unicast_connection.remote_port);

    etimer_set(&periodic_timer, SEND_INTERVAL);

	//static unsigned int message_number = 0;
    while(1){

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));							//wait 60 seconds
        printf("\n\n\n\nDEBUGGING: periodic timer expired\n");
        etimer_reset(&periodic_timer);
        etimer_set(&send_timer, SEND_TIME);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
        printf("\n\nDEBUGGING: send timer expired\n\n");
        addr = servreg_hack_lookup(SERVICE_ID);


        if(addr != NULL) {
			//static unsigned int message_number;

			printf("Sending MLE Link Request to ");
			uip_debug_ipaddr_print(addr);				//Prints address of receiver, check this
			printf("\n");
			//message_number++;

			uip_ipaddr_copy(&unicast_connection.remote_addr, addr);

			outputbuf_length = link_request_out(outputbuf);
	        printf("DEBUGGING remote ADDRESS IS ");
			uip_debug_ipaddr_print(&unicast_connection.remote_addr);


			//printf("\nDEBUGGING remote port is %d\n",(unsigned int)unicast_connection.remote_addr);

			//outputbuf[message_number] = 987;
			for(i=0;i<outputbuf_length;i++){

				printf("%u  ",outputbuf[i]);

			}
			printf("\n");



			simple_udp_sendto(&unicast_connection, outputbuf, outputbuf_length,addr);			//Check this. Check if +1 is needed.

			//So that LR is only sent once
			break;
		}
		else{
	  	   	printf("Service %d not found\n", SERVICE_ID);
	  	}

    }
    printf("DEBUGGING: Sender outside the loop");
//    uip_udp_remove(unicast_connection.udp_conn);
//    simple_udp_register(&unicast_connection, UDP_PORT,
//                        NULL, UDP_PORT, data_received_sender);


//        etimer_reset(&periodic_timer);
//        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer))
//
//        ;
//   	    printf("\n\n\n\nDEBUGGING: time expired before register\n\n\n");
//	simple_udp_register(&unicast_connection, UDP_PORT,
//									  NULL, UDP_PORT, data_received_sender);
//	printf("\n\n\n\nDEBUGGING: after register\n\n\n");

//	while(1){
//
//
//
//			PROCESS_WAIT_EVENT();
//	  	    printf("\n\n\n\nDEBUGGING: EVENT\n\n\n");
////			simple_udp_register(&unicast_connection, UDP_PORT,
////											  NULL, UDP_PORT, receiver);						//Trying with receiver first
//
//
////		    etimer_reset(&periodic_timer);
////		    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));							//wait 60 seconds
////		    printf("\n\n\n\nDEBUGGING: periodic timer expired\n");
////
////		    etimer_set(&send_timer, SEND_TIME);
////		    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
////		    printf("\n\nDEBUGGING: send timer expired\n\n");
//
//
//
//  	    }




  	PROCESS_END();
}

