/**************************************************************
 * main.c
 * rev 1.0 04-Oct-2022 justi
 * ATest
 * ***********************************************************/

#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include "terminal.h"
#include "hardware/irq.h"

#define UART_ID uart0
volatile char buffer[100];
volatile unsigned int indexx = 0;

#define Spindle 22

#define Xhome 2
#define Xstep 3
#define Xdir 4
#define Xfault 5

#define Yhome 6
#define Ystep 7
#define Ydir 8
#define Yfault 9

#define Zhome 10
#define Zstep 11
#define Zdir 12
#define Zfault 13

#define Mode0 21
#define Mode1 20
#define Mode2 19
#define Sleep 18

#define SW1 14
#define SW2 15
#define SW3 17
#define SW4 16
#define MAX_LINE_LENGTH 1000

int mode_2_step(int move, int step, int place, int dir)
{
  if (move > 0)
  {
    gpio_put(step, true);
    return 1;
  }
  else if (move < 0)
  {
    gpio_put(dir, true);
    gpio_put(step, true);
    return -1;
  }
  else
  {
    return 0;
  }
}

int debug_step(step, dir)
{
  if ((!gpio_get(SW4)) && (!gpio_get(SW3)))
  {
    return 0;
  }
  if (!gpio_get(SW4))
  {
    gpio_put(step, true);
    sleep_ms(1);
    gpio_put(step, false);
    sleep_ms(1);
    return 1;
  }
  else if (!gpio_get(SW3))
  {
    gpio_put(dir, true);
    gpio_put(step, true);
    sleep_ms(1);
    gpio_put(step, false);
    gpio_put(dir, false);
    sleep_ms(1);
    return -1;
  }
  else
  {
    return 0;
  }
}

int main(void)
{
  ////////////////////////////////////////////////////
  ////////////////////////////////////////////////////

  // TODO - Initialise components and variables
  stdio_init_all();
  gpio_init(Spindle);
  gpio_set_dir(Spindle, true);

  gpio_init(Xhome);
  gpio_set_dir(Xhome, false);
  gpio_init(Xstep);
  gpio_set_dir(Xstep, true);
  gpio_init(Xdir);
  gpio_set_dir(Xdir, true);
  gpio_init(Xfault);
  gpio_set_dir(Xfault, false);

  gpio_init(Yhome);
  gpio_set_dir(Yhome, false);
  gpio_init(Ystep);
  gpio_set_dir(Ystep, true);
  gpio_init(Ydir);
  gpio_set_dir(Ydir, true);
  gpio_init(Yfault);
  gpio_set_dir(Yfault, false);

  gpio_init(Zhome);
  gpio_set_dir(Zhome, false);
  gpio_init(Zstep);
  gpio_set_dir(Zstep, true);
  gpio_init(Zdir);
  gpio_set_dir(Zdir, true);
  gpio_init(Zfault);
  gpio_set_dir(Zfault, false);

  gpio_init(Mode0);
  gpio_set_dir(Mode0, true);
  gpio_init(Mode1);
  gpio_set_dir(Mode1, true);
  gpio_init(Mode2);
  gpio_set_dir(Mode2, true);
  gpio_init(Sleep);
  gpio_set_dir(Sleep, true);

  gpio_init(SW1);
  gpio_init(SW2);
  gpio_init(SW3);
  gpio_init(SW4);

  const uint green = PICO_DEFAULT_LED_PIN;
  gpio_init(green);
  gpio_set_dir(green, 1);

  int x_move = 0;
  int x_place = 0;
  int x_edge = 0;
  int x_time = 0;
  int x_timer = 0;

  int y_move = 0;
  int y_place = 0;
  int y_edge = 0;
  int y_time = 0;
  int y_timer = 0;

  int z_move = 0;
  int z_place = 0;
  int z_edge = 0;
  int z_time = 0;
  int z_timer = 0;

  int max_steps = 0;
  int codeType = 0;
  int debug = 0;
  int debug_target = 0;

  int temp_move = 0;
  int cur_step = 0;

  int xlist[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int ylist[16] = {0, 250, 250, 250, 0, 0, -250, 0, 0, -250, -250, -250, 0, 0, 250, 0, 0};
  int zlist[16] = {0, 0, 250, 0, 0, 250, 250, 250, 0, 0, -250, 0, 0, -250, -250, -250, 0};

  gpio_put(Sleep, 1);

  while (codeType == 0)
  {
    if (!gpio_get(SW1) + !gpio_get(SW2) + !gpio_get(SW3) + !gpio_get(SW4) > 1)
    {
      codeType = 0;
    }
    else if (!gpio_get(SW1))
    {
      codeType = 1;
    }
    else if (!gpio_get(SW2))
    {
      codeType = 2;
    }
    else if (!gpio_get(SW3))
    {
      codeType = 3;
    }
    else if (!gpio_get(SW4))
    {
      codeType = 4;
    }
  }
  while (true)
  {
    // TODO - Repeated code here
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    while (codeType == 2)
    {
      x_move = xlist[cur_step] - x_place;
      y_move = ylist[cur_step] - y_place;
      z_move = zlist[cur_step] - z_place;
      cur_step++;
      while (abs(x_move) + abs(y_move) + abs(z_move) != 0)
      {
        max_steps = MAX(abs(x_move), MAX(abs(y_move), abs(z_move)));
        x_time = abs(max_steps / x_move);
        y_time = abs(max_steps / y_move);
        z_time = abs(max_steps / z_move);
        //////////////////////////////////////////////////////////////////////////
        x_timer++;
        y_timer++;
        z_timer++;
        temp_move = 0;
        if (x_timer >= x_time)
        { // move place
          temp_move = mode_2_step(x_move, Xstep, x_place, Xdir);
          x_move = x_move - temp_move;
          x_place = x_place + temp_move;
          x_timer = 0;
        }
        if (y_timer >= y_time)
        {
          temp_move = mode_2_step(y_move, Ystep, y_place, Ydir);
          y_move = y_move - temp_move;
          y_place = y_place + temp_move;
          y_timer = 0;
        }
        if (z_timer >= z_time)
        {
          temp_move = mode_2_step(z_move, Zstep, z_place, Zdir);
          z_move = z_move - temp_move;
          z_place = z_place + temp_move;
          z_timer = 0;
        }
        sleep_ms(1);
        gpio_put(Xstep, false);
        gpio_put(Xdir, false);
        gpio_put(Ystep, false);
        gpio_put(Ydir, false);
        gpio_put(Zstep, false);
        gpio_put(Zdir, false);
        sleep_ms(1);
      }
    }
    while (codeType == 1)
    {
      int debug = 1;
      while (debug == 1)
      {
        if (!gpio_get(SW2))
        {
          debug_target++;
          debug_target = ((debug_target) > (3) ? (0) : debug_target);
          for (int i = 0; i < (debug_target + 1); i++)
          {
            sleep_ms(150);      // Wait for a while
            gpio_put(green, 1); // Switch on the LED
            sleep_ms(150);      // Wait for a while
            gpio_put(green, 0); // Switch off the LED
          }
        }
        if (!gpio_get(SW1))
        {
          // options
        }
        if (debug_target == 0)
        {
          x_place = x_place + debug_step(Xstep, Xdir);
        }
        if (debug_target == 1)
        {
          y_place = y_place + debug_step(Ystep, Ydir);
        }
        if (debug_target == 2)
        {
          z_place = z_place + debug_step(Zstep, Zdir);
        }
        if (debug_target == 3)
        {
          //////turn on the spindle TODO
        }
      }
    }
    while (codeType == 3)
    {
      int ch = getchar_timeout_us(0);
      if ((ch == '\r') || (ch == '\n') || (indexx == 99))
      {
        buffer[indexx] = 0;
        indexx = 0;
        printf("\n\r");
        int volatile val = 0;
        if (sscanf(buffer, "x %hi", &val) > 0)
        {
          printf("x");
          x_move = val - x_place;
          printf("%d", x_move);
        }
        //        int volatile val = 0;
        if (sscanf(buffer, "y %hi", &val) > 0)
        {
          printf("y");
          y_move = val - y_place;
        }
        //       int volatile val = 0;
        if (sscanf(buffer, "z %hi", &val) > 0)
        {
          printf("z");
          z_move = val - z_place;
        }
        //////////////////////////////////////////////////////////////////////////
        while (abs(x_move) + abs(y_move) + abs(z_move) != 0)
        {
          max_steps = MAX(abs(x_move), MAX(abs(y_move), abs(z_move)));
          x_time = abs(max_steps / x_move);
          y_time = abs(max_steps / y_move);
          z_time = abs(max_steps / z_move);
          x_timer++;
          y_timer++;
          z_timer++;
          temp_move = 0;
          if (x_timer >= x_time)
          { // move place
            temp_move = mode_2_step(x_move, Xstep, x_place, Xdir);
            x_move = x_move - temp_move;
            x_place = x_place + temp_move;
            x_timer = 0;
          }
          if (y_timer >= y_time)
          {
            temp_move = mode_2_step(y_move, Ystep, y_place, Ydir);
            y_move = y_move - temp_move;
            y_place = y_place + temp_move;
            y_timer = 0;
          }
          if (z_timer >= z_time)
          {
            temp_move = mode_2_step(z_move, Zstep, z_place, Zdir);
            z_move = z_move - temp_move;
            z_place = z_place + temp_move;
            z_timer = 0;
          }
          sleep_ms(1);
          gpio_put(Xstep, false);
          gpio_put(Xdir, false);
          gpio_put(Ystep, false);
          gpio_put(Ydir, false);
          gpio_put(Zstep, false);
          gpio_put(Zdir, false);
          sleep_ms(1);
        }
        /////////////////////////////////////////////////////////////
      }
      else if (ch == '\177')
      {
        if (indexx != 0)
        {
          buffer[indexx] = ch;
          indexx--;
          printf("%c", ch);
        }
      }
      else if (ch == '\f')
      {
        buffer[indexx] = 0;
        indexx = 0;
        printf("%c", ch);
      }
      else if (ch != -1)
      {
        buffer[indexx] = ch;
        indexx++;
        printf("%c", ch);
      }
      /*
            __asm("wfi");
            if (write == 1)
            {
              term_set_color(clrYellow, clrBlack);
              term_cls();
              term_move_to(1, 1);
              printf("CC2511 Lab 7\n\n");
              term_set_color(clrBlack, clrYellow);
              printf("+--[ PMW Status ]--+");
              term_set_color(clrBlack, clrBlack);
              printf(" ");
              term_set_color(clrBlack, clrYellow);
              printf("+-----------------[ How to use ]-----------------+\n ");

              for (int i = 0; i < 7; i++)
              {
                term_set_color(clrBlack, clrBlack);
                printf("                  ");
                term_set_color(clrBlack, clrYellow);
                printf(" ");
                term_set_color(clrBlack, clrBlack);
                printf(" ");
                term_set_color(clrBlack, clrYellow);
                printf(" ");
                term_set_color(clrBlack, clrBlack);
                printf("                                                ");
                term_set_color(clrBlack, clrYellow);
                printf(" \n ");
              }

              term_set_color(clrBlack, clrYellow);
              printf("                   ");
              term_set_color(clrBlack, clrBlack);
              printf(" ");
              term_set_color(clrBlack, clrYellow);
              printf("                                                  ");

              term_set_color(clrWhite, clrBlack);
              term_move_to(24, 5);
              printf("Type the following commands:");
              term_move_to(24, 6);
              printf("> red n       Set the red PWM ratio to n");
              term_move_to(24, 7);
              printf("> green n     Set the green PWM ratio to n");
              term_move_to(24, 8);
              printf("> blue n      Set the blue PWM ratio to n");
              term_move_to(24, 9);
              printf("> off         Turn all LEDs off");

              term_move_to(3, 5);
              printf("Red = %hhu\n", x_place);
              term_move_to(3, 6);
              printf("Green = %hhu\n", y_place);
              term_move_to(3, 7);
              printf("Blue = %hhu\n", z_place);

              term_move_to(1, 15);
              printf("Command prompt:\n> ");
              write = 0;
            }
            else if (read == 1)
            {
              int volatile val = 0;
              if (sscanf(buffer, "x %hi", &val) > 0)
              {
                x_place = val;
              }
              //        int volatile val = 0;
              if (sscanf(buffer, "y %hi", &val) > 0)
              {
                y_place = val;
              }
              //       int volatile val = 0;
              if (sscanf(buffer, "z %hi", &val) > 0)
              {
                z_place = val;
              }
            }*/
    }
  }
}
