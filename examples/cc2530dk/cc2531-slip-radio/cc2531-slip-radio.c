/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         Slip-radio driver
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */
#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/slip.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"
#include "cmd.h"
#include "packetutils.h"

#include "debug.h"

#include <string.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/*
 * Blink one LED on RF TX commands. Blink the second LED on reception of all
 * other commands
 */
#define CC2531_SLIP_RADIO_CMD_RFTX_LED  LEDS_GREEN
#define CC2531_SLIP_RADIO_CMD_MGMT_LED  LEDS_RED
/*---------------------------------------------------------------------------*/
void slip_send_packet(const uint8_t *ptr, int len);
int no_framer_parse(void);
/*---------------------------------------------------------------------------*/
static uint8_t packet_ids[16];

static int packet_pos;
static uint8_t buf[20];
/*---------------------------------------------------------------------------*/
/*
 * 6lbr slip_reboot workaround - See 6lbr issue #55
 * https://github.com/cetic/6lbr/issues/55
 */
static uint8_t hardware_mac[8];
/*---------------------------------------------------------------------------*/
static void
update_mac(const uint8_t *mac) CC_NON_BANKED
{
  if(NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, mac, 8)
     != RADIO_RESULT_OK) {
    putstring("cc2531: Error updating MAC.\n");
    putstring("cc2531: You may have to manually reboot the dongle.\n");
  }
  memcpy(&linkaddr_node_addr, mac, LINKADDR_SIZE);
  memcpy(&uip_lladdr.addr, &linkaddr_node_addr, sizeof(uip_lladdr.addr));
}
/*---------------------------------------------------------------------------*/
static int
cmd_handler_config(const uint8_t *data, int len)
{
  leds_on(CC2531_SLIP_RADIO_CMD_MGMT_LED);

  if(data[0] == '!') {
    if(data[1] == 'C') {
      putstring("cc2531: Setting RF channel 0x");
      puthex(data[2]);
      putstring("\n");

      if(NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, data[2])
         != RADIO_RESULT_OK) {
        putstring("cc2531: Error\n");
      }

      leds_off(CC2531_SLIP_RADIO_CMD_MGMT_LED);

      return 1;
    } else if(data[1] == 'R') {
      /*
       * This is a reboot command, and we will normally receive this if we are
       * running as the dumb radio for a native 6lbr.
       *
       * We do _not_ actually reboot due to the problems documented in 6lbr's
       * issue #55. Instead, we just revert to the hardware MAC.
       */
      putstring("cc2531: Reboot requested\n");
      putstring("cc2531: Reverting to h/w MAC but not rebooting\n");

      update_mac(hardware_mac);

      leds_off(CC2531_SLIP_RADIO_CMD_MGMT_LED);

      return 1;
    } else if(data[1] == 'M') {

      /* Update RF registers and Contiki's data structures with the new MAC */
      update_mac(&data[2]);

      leds_off(CC2531_SLIP_RADIO_CMD_MGMT_LED);

      return 1;
    }
  } else if(data[0] == '?') {
    if(data[1] == 'C') {
      radio_value_t chan;

      if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan)
         == RADIO_RESULT_OK) {
        buf[0] = '!';
        buf[1] = 'C';
        buf[2] = (uint8_t)chan;

        cmd_send(buf, 3);
      } else {
        putstring("cc2531: Read RF channel error\n");
      }

      leds_off(CC2531_SLIP_RADIO_CMD_MGMT_LED);

      return 1;
    } else if(data[1] == 'M') {
      if(NETSTACK_RADIO.get_object(RADIO_PARAM_64BIT_ADDR, &buf[2], 8)
         == RADIO_RESULT_OK) {
        buf[0] = '!';
        buf[1] = 'M';

        cmd_send(buf, 10);
      } else {
        putstring("cc2531: Read MAC Error\n");
      }

      leds_off(CC2531_SLIP_RADIO_CMD_MGMT_LED);
      return 1;
    }
  }
  leds_off(CC2531_SLIP_RADIO_CMD_MGMT_LED);
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
packet_sent(void *ptr, int status, int transmissions)
{
  uint8_t sid;
  int pos;
  sid = *((uint8_t *)ptr);

  PRINTF("cc2531: packet sent! sid: %d, status: %d, tx: %d\n",
         sid, status, transmissions);

  pos = 0;
  buf[pos++] = '!';
  buf[pos++] = 'R';
  buf[pos++] = sid;
  buf[pos++] = status; /* one byte ? */
  buf[pos++] = transmissions;
  cmd_send(buf, pos);
}
/*---------------------------------------------------------------------------*/
static int
cmd_handler_rf_tx(const uint8_t *data, int len)
{
  if(data[0] == '!') {
    /* should send out stuff to the radio - ignore it as IP */
    /* --- s e n d --- */
    if(data[1] == 'S') {
      int pos;

      leds_on(CC2531_SLIP_RADIO_CMD_RFTX_LED);
      packet_ids[packet_pos] = data[2];

      packetbuf_clear();
      pos = packetutils_deserialize_atts(&data[3], len - 3);
      if(pos < 0) {
        PRINTF("cc2531: illegal packet attributes\n");
        leds_off(CC2531_SLIP_RADIO_CMD_RFTX_LED);
        return 1;
      }
      pos += 3;
      len -= pos;
      if(len > PACKETBUF_SIZE) {
        len = PACKETBUF_SIZE;
      }
      memcpy(packetbuf_dataptr(), &data[pos], len);
      packetbuf_set_datalen(len);

      PRINTF("cc2531: sending %u (%d bytes)\n",
             data[2], packetbuf_datalen());

      /* parse frame before sending to get addresses, etc. */
      no_framer_parse();
      NETSTACK_MAC.send(packet_sent, &packet_ids[packet_pos]);

      packet_pos++;
      if(packet_pos >= sizeof(packet_ids)) {
        packet_pos = 0;
      }

      leds_off(CC2531_SLIP_RADIO_CMD_RFTX_LED);
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
cc2531_slip_radio_cmd_output(const uint8_t *data, int data_len)
{
  slip_send_packet(data, data_len);
}
/*---------------------------------------------------------------------------*/
static void
slip_input_callback(void)
{
  PRINTF("cc2531: %u '%c%c'\n", uip_len, uip_buf[0], uip_buf[1]);
  cmd_input(uip_buf, uip_len);
  uip_len = 0;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  process_start(&slip_process, NULL);
  slip_set_input_callback(slip_input_callback);
  packet_pos = 0;
}
/*---------------------------------------------------------------------------*/
PROCESS(cc2531_slip_radio_process, "CC2531 Slip radio process");
AUTOSTART_PROCESSES(&cc2531_slip_radio_process);
/*---------------------------------------------------------------------------*/
CMD_HANDLERS(cmd_handler_config, cmd_handler_rf_tx);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc2531_slip_radio_process, ev, data)
{

  PROCESS_BEGIN();

  init();

  NETSTACK_RDC.off(1);

  /* Save the hardware MAC so we can restore it later if required */
  memcpy(hardware_mac, &linkaddr_node_addr, LINKADDR_SIZE);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
