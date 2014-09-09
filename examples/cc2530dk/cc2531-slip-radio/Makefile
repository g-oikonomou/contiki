CONTIKI_PROJECT=cc2531-slip-radio

all: $(CONTIKI_PROJECT)

APPS = slip-cmd

ifeq ($(TARGET),)
  -include Makefile.target
endif

CONTIKI=../../..

UIP_CONF_IPV6=1
UIP_CONF_RPL=0

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

PROJECT_SOURCEFILES += slip-net.c no-framer.c

include $(CONTIKI)/Makefile.include
