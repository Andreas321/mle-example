all: mle-send mle-recieve mle-node-A mle-node-B mle-receive-new

APPS=servreg-hack
CONTIKI=../../..

CONTIKI_WITH_IPV6 = 1
CONTIKI_WITH_MLE =1
include $(CONTIKI)/Makefile.include
