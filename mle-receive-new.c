/*
 * mle-receive.c
 *
 *  Created on: Mar 1, 2017
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

//#include "net/rpl/rpl.h"

#include <stdio.h>
#include <string.h>





#define UDP_PORT 19788					//check if mle receives on this port
#define SERVICE_ID 190

#define SEND_INTERVAL		(60 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))			//Double check what this time should be

static struct simple_udp_connection unicast_connection;				//Check if this needs to be adapted for MLE


uint8_t outputbuf[30];
uint8_t outputbuf_length = 0;
uint8_t flag =0;
/*---------------------------------------------------------------------------*/
PROCESS(mle_receiver_process, "MLE received link request example process");
AUTOSTART_PROCESSES(&mle_receiver_process);
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
  printf("Data received from ");
  uip_debug_ipaddr_print(sender_addr);
  printf(" on port %d from port %d with length %d: '%s'\n",						//%s not working, probably because uint8_t not char
         receiver_port, sender_port, datalen, data);
  //My addition
  uint8_t i=0;
  printf("\n");
  for(i=0;i<datalen;i++){
	  printf("%d ",data[i]);
  }
  printf("\n");
//  printf("\nSending Received message\n");
//  simple_udp_sendto(c, "MESSAGE RECEIVED", 17, sender_addr);

}
/*--------------------------------------------------------------------------*/
//My own function simple udp register will call back this function so that data and length maybe extracted
void data_received(struct simple_udp_connection *c,
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

	 //Maybe this could go in a new function called command_reader?
	 switch(data[1]){

	 	 case 0:							//Link Request
	 		//outputbuf_length = link_request_in(data,datalen,outputbuf);
	 		outputbuf_length = link_request_in(data,datalen,outputbuf);
	 		//uip_create_unspecified(&unicast_connection.remote_addr);
			simple_udp_sendto(&unicast_connection, outputbuf, outputbuf_length, sender_addr);
		    printf("\nSent data is: ");
			uint8_t i;
			for(i=0;i<outputbuf_length;i++){

				printf("%u  ",outputbuf[i]);

			}
			printf("\n");

			break;

	 	 case 1:							//Link Accept
	 		//outputbuf_length = link_accept_in(data,datalen,outputbuf);
	 		 break;
	 	 case 2:							//Link Reject
	 		 break;
	 	 case 3:							//Link Accept and Request
	 		 //outputbuf_length = link_accept_and_request_in(data,datalen,outputbuf);
	 		 break;
	 	 case 4:							//Advertisement
	 		advertisement_handler(data,datalen);
	 		receiver(c, sender_addr, sender_port, receiver_addr, receiver_port, data, datalen);
	 		break;
	 	 case 5:							//Update
	 		 break;
	 	 case 6:							//Update request
	 		 break;

	 }

		//	flag =1;


	 //return outputbuf_length;
}
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t *
set_global_address(void)
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
/*---------------------------------------------------------------------------*/
//static void
//create_rpl_dag(uip_ipaddr_t *ipaddr)
//{
//  struct uip_ds6_addr *root_if;
//
//  root_if = uip_ds6_addr_lookup(ipaddr);
//  if(root_if != NULL) {
//    rpl_dag_t *dag;
//    uip_ipaddr_t prefix;
//
//    rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
//    dag = rpl_get_any_dag();
//    uip_ip6addr(&prefix, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
//    rpl_set_prefix(dag, &prefix, 64);
//    PRINTF("created a new RPL dag\n");
//    printf("created a new RPL dag\n");													//added as uppercase printf did not appear on cooja
//  } else {
//    PRINTF("failed to create a new RPL DAG\n");
//    printf("failed to create a new RPL DAG\n");											//added as uppercase printf did not appear on cooja
//  }
//}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mle_receiver_process, ev, data)
{
  uip_ipaddr_t *ipaddr;
  uip_ipaddr_t *addr;
  uint8_t data_in;
  static struct etimer send_timer;
  static struct etimer periodic_timer;


  PROCESS_BEGIN();

  servreg_hack_init();

  ipaddr = set_global_address();

//  create_rpl_dag(ipaddr);

  servreg_hack_register(SERVICE_ID, ipaddr);

//  simple_udp_register(&unicast_connection, UDP_PORT,
//                      	  NULL, UDP_PORT, receiver);


 simple_udp_register(&unicast_connection, UDP_PORT,
                     NULL, UDP_PORT, data_received);


  while(1) {
	  PROCESS_WAIT_EVENT();

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/



