/*
 * sddc_test - simple test program for libsddc
 *
 * Copyright (C) 2020 by Franco Venturi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>

#include "libsddc.h"

#if _WIN32
#include <windows.h>
#define sleep(x) Sleep(x*1000)
#else
#include <unistd.h>
#endif

static void blink_led(libsddc_handler_t sddc, uint8_t color);


int main(int argc, char **argv)
{
  if (argc != 1) {
    fprintf(stderr, "usage: %s\n", argv[0]);
    return -1;
  }

  /* count devices */
  int count = sddc_get_device_count();
  if (count < 0) {
    fprintf(stderr, "ERROR - sddc_get_device_count() failed\n");
    return -1;
  }
  printf("device count=%d\n", count);

  /* get device info */
  struct sddc_device_t sddc_device_infos;
  for(int i = 0; i < count; ++i)
  {
    sddc_get_device(i, &sddc_device_infos);
    printf("%d - product=\"%s\" serial number=\"%s\"\n",
          i,
          sddc_device_infos.product,
          sddc_device_infos.serial_number);
  }

  /* open and close device */
  libsddc_handler_t sddc = sddc_create();
  sddc_err_t ret = sddc_init(sddc, 0);

  if (ret != ERR_SUCCESS) {
    fprintf(stderr, "ERROR - sddc_open() failed\n");
    return -1;
  }

  /* blink the LEDs */
  printf("blinking the red LED\n");
  blink_led(sddc, SDDC_LED_RED);
  printf("blinking the yellow LED\n");
  blink_led(sddc, SDDC_LED_YELLOW);
  printf("blinking the blue LED\n");
  blink_led(sddc, SDDC_LED_BLUE);

  /* done */
  sddc_destroy(sddc);

  return 0;
}

static void blink_led(libsddc_handler_t sddc, uint8_t color)
{
  for (int i = 0; i < 5; ++i) {
    sddc_err_t ret = sddc_set_led(sddc, color, true);
    if (ret != ERR_SUCCESS) {
      fprintf(stderr, "ERROR - sddc_led_on(%02x) failed\n", color);
      return;
    }
    sleep(1);
    ret = sddc_set_led(sddc, color, false);
    if (ret != ERR_SUCCESS) {
      fprintf(stderr, "ERROR - sddc_led_off(%02x) failed\n", color);
      return;
    }
    sleep(1);
  }
  return;
}