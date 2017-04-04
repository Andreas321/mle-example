/*
 * mle-advert-mcast.c
 *
 *  Created on: Mar 17, 2017
 *      Author: user
 */

/*																		//Taken from udp-client.c
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

The choice of server address determines its 6LoWPAN header compression.
 * (Our address will be compressed Mode 3 since it is derived from our
 * link-local address)
 * Obviously the choice made here must also be selected in udp-server.c.
 *
 * For correct Wireshark decoding using a sniffer, add the /64 prefix to the
 * 6LowPAN protocol preferences,
 * e.g. set Context 0 to fd00::. At present Wireshark copies Context/128 and
 * then overwrites it.
 * (Setting Context 0 to fd00::1111:2222:3333:4444 will report a 16 bit
 * compressed address of fd00::1111:22ff:fe33:xxxx)
 *
 * Note the IPCMV6 checksum verification depends on the correct uncompressed
 * addresses.
 *

#if 0
// Mode 1 - 64 bits inline
   uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 1);
#elif 1
//Mode 2 - 16 bits inline
  uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
#else
//Mode 3 - derived from server link-local (MAC) address
  uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0x0250, 0xc2ff, 0xfea8, 0xcd1a); //redbee-econotag
#endif
}
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



#define UDP_PORT 19788					//unicast-sender uses 1234
#define SERVICE_ID 190

#define SEND_INTERVAL		(60 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))			//Double check what this time should be

static struct simple_udp_connection multicast_connection;				//Check if this needs to be adapted for MLE

PROCESS(mle_multicast_advert,"mle multicast advert");
AUTOSTART_PROCESSES(&mle_multicast_advert);
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
/*---------------------------------------------------------------------------*/
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
/*static void
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
}*/
/*------------------------------------------------------------------------------*/
PROCESS_THREAD(mle_multicast_advert, ev, data)
{

	static struct etimer periodic_timer;
	static struct etimer send_timer;
	uip_ipaddr_t addr;																	//Not declared as pointer like in unicast_sender
  	PROCESS_BEGIN();

    //servreg_hack_init();
  	//ipaddr = set_global_address();
    //set_global_address();																//This has been removed, should i put it back?: will it conflict with SourceAddress TLV
    simple_udp_register(&multicast_connection, UDP_PORT,
                        NULL, UDP_PORT, receiver);


    etimer_set(&periodic_timer, SEND_INTERVAL);
  	while(1){
  	    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  	    etimer_reset(&periodic_timer);
  	    etimer_set(&send_timer, SEND_TIME);													//May have to change SEND time to 30 seconds to fit like MLE on RIP maybe include etimer_set(&et, START_DELAY * CLOCK_SECOND);

  	    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));


  	    printf("Sending multicast\n");
  	    uip_create_linklocal_allnodes_mcast(&addr);
        uip_debug_ipaddr_print(&addr);
        printf("\n");

        uint8_t outputbuf[30] = {0};				//Look at changing this to a MACRO default.  Is 108 the maximum?
        uint16_t outputbuf_length;					//Why did I decide on 16 bit?

		outputbuf_length = advertisement_out(outputbuf);

		uint8_t i;
		for(i=0;i<outputbuf_length;i++){

			printf("%u  ",outputbuf[i]);

		}
		printf("\n");
  	    simple_udp_sendto(&multicast_connection, outputbuf, outputbuf_length, &addr);

  	}


  	PROCESS_END();
}

