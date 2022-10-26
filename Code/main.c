/**************************************************************
 * main.c
 * rev 1.0 04-Oct-2022 justi
 * ATest
 * ***********************************************************/

#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include "terminal.h"
#include "hardware/irq.h"

#define sleep_time 50

#define UART_ID uart0
char buffer[100];
unsigned int indexx = 0;

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

#define GREEN_LED PICO_DEFAULT_LED_PIN
// When using this one need to set both dir and step to false after a sleep command, returns 1 or -1 depending on direction.
int one_step_in_dir(int *move, int step, int dir, int max, int *place, bool debug)
{
  if ((*move > 0) && ((*place < max) || (debug)))
  {
    gpio_put(step, true);
    *move = *move - 1;
    *place = *place + 1;
  }
  else if ((*move < 0) && ((*place > 0) || (debug)))
  {
    gpio_put(dir, true);
    gpio_put(step, true);
    *move = *move + 1;
    *place = *place - 1;
  }
  else if (!(debug))
  {
    *move = 0;
  }
}

// When using this one no need to set any sleep command, returns 1 or -1 depending on direction.
int debug_step(int step, int dir)
{
  if ((!gpio_get(SW4)) && (!gpio_get(SW3))) // negate both buttons pushed
  {
    return 0;
  }
  if (!gpio_get(SW4))
  {
    gpio_put(step, true);
    sleep_us(sleep_time);
    gpio_put(step, false);
    sleep_us(sleep_time);
    return 1;
  }
  else if (!gpio_get(SW3))
  {
    gpio_put(dir, true);
    gpio_put(step, true);
    sleep_us(sleep_time);
    gpio_put(step, false);
    gpio_put(dir, false);
    sleep_us(sleep_time);
    return -1;
  }
  else
  {
    return 0;
  }
}

// Blinks green light amount times
void blink(int amount)
{
  for (int i = 0; i < (amount); i++)
  {
    sleep_ms(100);
    gpio_put(GREEN_LED, 1);
    sleep_ms(100);
    gpio_put(GREEN_LED, 0);
  }
  sleep_ms(300);
}

// Main step code, moves until x_move, y_move and z_move all equil zero, and in straight line.
void moving_steps(int *x_move, int *y_move, int *z_move, int *x_place, int *y_place, int *z_place, int x_max, int y_max, int z_max, bool debug)
{
  int x_time = 0;
  int x_timer = 0;
  int y_time = 0;
  int y_timer = 0;
  int z_time = 0;
  int z_timer = 0;
  int max_steps = 0;
  int temp_move = 0;
  while (abs(*x_move) + abs(*y_move) + abs(*z_move) != 0)
  {
    max_steps = MAX(abs(*x_move), MAX(abs(*y_move), abs(*z_move)));
    x_time = abs(max_steps / *x_move);
    y_time = abs(max_steps / *y_move);
    z_time = abs(max_steps / *z_move);
    x_timer++;
    y_timer++;
    z_timer++;
    temp_move = 0;
    if (x_timer >= x_time)
    {
      one_step_in_dir(&*x_move, Xstep, Xdir, x_max, &*x_place, debug);
      x_timer = 0;
    }
    if (y_timer >= y_time)
    {
      one_step_in_dir(&*y_move, Ystep, Ydir, y_max, &*y_place, debug);
      y_timer = 0;
    }
    if (z_timer >= z_time)
    {
      one_step_in_dir(&*z_move, Zstep, Zdir, z_max, &*z_place, debug);
      z_timer = 0;
    }
    sleep_us(sleep_time);
    gpio_put(Xstep, false);
    gpio_put(Xdir, false);
    gpio_put(Ystep, false);
    gpio_put(Ydir, false);
    gpio_put(Zstep, false);
    gpio_put(Zdir, false);
    sleep_us(sleep_time);
  }
}

// Size is radius of circle and is drawn from the left side
void draw_circle(int size, int x_place, int y_place, int z_place, int x_max, int y_max, int z_max)
{
  int x_move = 0;
  int y_move = 0;
  int temp_x = x_place;
  int temp_y = y_place;
  int z_move = 0;
  for (int i = 0; i <= (4 * size); i++)
  {
    if (i <= (2 * size))
    {
      x_move = i - x_place + temp_x;
      y_move = sqrt(size * size - (i - size) * (i - size)) - y_place + temp_y;
    }
    else
    {
      x_move = 4 * size - i - x_place + temp_x;
      y_move = 0 - sqrt(size * size - (3 * size - i) * (3 * size - i)) - y_place + temp_y;
    }
    moving_steps(&x_move, &y_move, &z_move, &x_place, &y_place, &z_place, x_max, y_max, z_max, 0);
  }
}

// Prints different values depending on input (0-3)
void print_debug_target(int target)
{
  if (target == 0)
  {
    printf("Motor: x      \n\r");
  }
  else if (target == 1)
  {
    printf("Motor: y      \n\r");
  }
  else if (target == 2)
  {
    printf("Motor: z      \n\r");
  }
  else if (target == 3)
  {
    printf("Motor: Spindle\n\r");
  }
}

int main(void)
{
  // Initialise components and variables
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

  int Spindle_slice = pwm_gpio_to_slice_num(Spindle);

  pwm_set_clkdiv(Spindle_slice, 256);
  gpio_set_function(Spindle, GPIO_FUNC_PWM);
  pwm_set_wrap(Spindle_slice, 255);
  pwm_set_enabled(Spindle_slice, 1);

  gpio_init(GREEN_LED);
  gpio_set_dir(GREEN_LED, 1);

  int x_move = 0;
  int x_place = 0;
  int x_edge = 0;

  int y_move = 0;
  int y_place = 0;
  int y_edge = 0;

  int z_move = 0;
  int z_place = 0;
  int z_edge = 0;

  int codeType = 0;
  int debug_target = 0;
  bool debug = 1;

  int cur_step = 0;
  bool spindle_on = 0;

  int x_max = 0;
  int y_max = 0;
  int z_max = 0;

  int origin[3] = {0, 0, 0};
  int gcode = 0;
  int scale = 1;

  // currently input of points to follow as x, y, z, repeated
  ////  static int list[] = {0, 190, 30, 0, 187, 32, 0, 184, 35, 0, 181, 37, 0, 178, 40, 0, 175, 43, 0, 170, 45, 0, 166, 47, 0, 162, 50, 0, 155, 54, 0, 151, 56, 0, 147, 58, 0, 143, 61, 0, 138, 63, 0, 132, 65, 0, 128, 68, 0, 124, 72, 0, 124, 68, 0, 124, 63, 0, 122, 60, 0, 121, 62, 0, 122, 65, 0, 122, 67, 0, 119, 68, 0, 116, 66, 0, 110, 66, 0, 105, 67, 0, 99, 68, 0, 95, 68, 0, 94, 62, 0, 92, 65, 0, 93, 68, 0, 95, 72, 0, 92, 70, 0, 89, 67, 0, 83, 65, 0, 80, 63, 0, 75, 60, 0, 70, 57, 0, 65, 54, 0, 62, 51, 0, 57, 46, 0, 52, 44, 0, 47, 43, 0, 42, 40, 0, 39, 37, 0, 37, 34, 0, 28, 32, 0, 29, 36, 0, 32, 41, 0, 34, 47, 0, 30, 45, 0, 35, 52, 0, 30, 54, 0, 38, 62, 0, 33, 66, 0, 47, 71, 0, 42, 75, 0, 55, 79, 0, 52, 82, 0, 62, 86, 0, 58, 89, 0, 72, 90, 0, 69, 94, 0, 82, 94, 0, 82, 98, 0, 92, 94, 0, 90, 91, 0, 86, 88, 0, 78, 88, 0, 72, 84, 0, 66, 81, 0, 63, 79, 0, 58, 75, 0, 55, 73, 0, 50, 67, 0, 44, 62, 0, 42, 57, 0, 42, 48, 0, 47, 52, 0, 53, 56, 0, 60, 60, 0, 65, 64, 0, 70, 67, 0, 74, 71, 0, 82, 74, 0, 90, 78, 0, 95, 85, 0, 95, 83, 0, 96, 79, 0, 96, 76, 0, 100, 73, 0, 103, 73, 0, 107, 76, 0, 111, 76, 0, 114, 74, 0, 118, 73, 0, 123, 79, 0, 124, 83, 0, 118, 86, 0, 112, 87, 0, 109, 91, 0, 106, 88, 0, 103, 88, 0, 97, 88, 0, 94, 88, 0, 94, 90, 0, 94, 96, 0, 96, 100, 0, 97, 105, 0, 95, 111, 0, 91, 116, 0, 95, 114, 0, 95, 121, 0, 102, 116, 0, 104, 126, 0, 107, 118, 0, 111, 125, 0, 114, 117, 0, 120, 124, 0, 119, 113, 0, 125, 119, 0, 119, 106, 0, 119, 98, 0, 116, 106, 0, 110, 108, 0, 105, 109, 0, 101, 106, 0, 97, 97, 0, 100, 101, 0, 105, 105, 0, 110, 106, 0, 115, 103, 0, 117, 101, 0, 120, 97, 0, 122, 91, 0, 125, 88, 0, 132, 86, 0, 138, 85, 0, 143, 83, 0, 154, 81, 0, 159, 75, 0, 163, 71, 0, 167, 66, 0, 170, 61, 0, 172, 54, 0, 172, 50, 0, 173, 46, 0, 180, 39, 0, 186, 33, 0, 185, 46, 0, 188, 46, 0, 182, 57, 0, 187, 56, 0, 176, 67, 0, 181, 68, 0, 167, 76, 0, 172, 79, 0, 154, 81, 0, 160, 85, 0, 140, 89, 0, 147, 92, 0, 131, 92, 0, 137, 96, 0, 121, 97};
  static int list[] = {0, 0, 0, 0, 1, 3.5, 0, 5, 4, 0, 7, 5, 0, 12, 10, 0, 13, 12, 0, 8, 11, 0, 4, 10, 0, -1, 7, 0, -2, 9, 0, -2.5, 8, 0, -3, 10, 0, -3.5, 9, 0, -5, 11, 0, -6, 10, 0, -8, 11, 0, -8, 9, 0, -9.5, 10, 0, -10, 8, 0, -11, 12, 0, -11, 15, 0, -14, 18, 0, -15.5, 20, 0, -16, 18, 0, -16, 13, 0, -14, 8, 0, -12, 6, 0, -13, 0, 0, -12, -2, 0, -9, -4, 0, -6, -5, 0, -2, -4, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -2, -4, 0, -6, -5, 0, -9, -4, 0, -12, -2, 0, -13, 0, 0, -15, -2, 0, -15, -5, 0, -13, -9, 0, -12, -11, 0, -7, -13, 0, -2, -13, 0, 1, -11, 0, 3, -5, 0, 2, -2, 0, 0, -1, 0, 2, -2, 0, 6, 3, 0, 10, 4, 0, 12, 5, 0, 15, 8, 0, 16, 3, 0, 15.5, 0, 0, 14, -5, 0, 11, -8, 0, 8, -9, 0, 5, -9, 0, 3, -7, 0, 5, -9, 0, 8, -9, 0, 11, -8, 0, 14, -5, 0, 15.5, 0, 0, 16, 3, 0, 15, 8, 0, 12, 5, 0, 10, 4, 0, 6, 3, 0, 2, -2, 0, 0, -1, 0, 2, -2, 0, 3, -5, 0, 1, -11, 0, 0.5, -11.5, 0, -1, -15, 0, -2, -17, 0, -3.5, -17, 0, -2, -17, 0, -1, -15, 0, 0.5, -11.5, 0, 1, -11, 0, 1, -15, 0, 0, -17, 0, 0, -18, 0, 2, -18, 0, 4, -17, 0, 5, -14, 0, 5, -9, 0, 5, -14, 0, 4, -17, 0, 2, -18, 0, 0, -18, 0, 0, -17, 0, 1, -15, 0, 1, -11, 0, -2, -13, 0, -3, -15, 0, -4, -18, 0, -6, -19.5, 0, -8, -18.5, 0, -7, -16, 0, -8, 18.5, 0, -9, -19, 0, -11, -18, 0, -10.5, -16, 0, -11, -13, 0, -11, -12};
  // makes set length of xyz list
  int xlist[sizeof(list) / 3];
  int ylist[sizeof(list) / 3];
  int zlist[sizeof(list) / 3];
  // splits main list into xyz
  for (int i = 0; i < (sizeof(list) - 3); i = i + 3)
  {
    xlist[i / 3] = (list[i] + 290) * 200;
    ylist[i / 3] = (list[i + 1] + 290) * 200;
    zlist[i / 3] = list[i + 2];
  }
  // write is for making the termanal the first time
  int write = 1;
  gpio_put(Sleep, 1);

  gpio_put(Mode0, 1);
  gpio_put(Mode1, 1);
  gpio_put(Mode2, 1);

  while (true)
  {
    term_cls();
    term_move_to(1, 1);
    printf("Button 1:    Debug Mode\n\rButton 2:    Terminal\n\rButton 3:    Currently not in use\n\rButton 4:    Currently not in use\n\r");
    while ((codeType < 1) || (codeType > 4)) // selects the codetype using the onboard buttons.
    {
      if (!gpio_get(SW1) + !gpio_get(SW2) + !gpio_get(SW3) + !gpio_get(SW4) > 1)
      {
        codeType = 0;
      }
      else if (!gpio_get(SW1))
      {
        codeType = 1;
        term_cls();
        term_move_to(1, 1);
        printf("Button 1:    Options\n\rButton 2:    Change Motor\n\rButton 3:    Move Negative\n\rButton 4:    Move Positive\n\n\r");
        print_debug_target(debug_target);
      }
      else if (!gpio_get(SW2))
      {
        codeType = 2;
        term_cls();
        term_move_to(1, 1);
      }
      else if (!gpio_get(SW3))
      {
        codeType = 3;
        term_cls();
        term_move_to(1, 1);
      }
      else if (!gpio_get(SW4))
      {
        codeType = 4;
        term_cls();
        term_move_to(1, 1);
      }
    }
    blink(codeType);
    while (codeType == 1) // debug using physical buttons.
    {
      while (debug == 1) // debug = 0 usedd to fall outof this loop, if codetype still 1 by end debug reactivated.
      {
        if (!gpio_get(SW2)) // changes what motor is being affected, (1-4)loop
        {
          debug_target++;
          debug_target = ((debug_target) > (3) ? (0) : debug_target); // debug_target = (if (debug_target > 3) {0} else {debug_target})
          term_move_to(1, 6);
          print_debug_target(debug_target);
          blink(debug_target + 1);
        }
        if (!gpio_get(SW1)) // fall out of debug into options
        {
          debug = 0;
          term_cls();
          term_move_to(1, 1);
          printf("Button 1:    Exit Options\n\rButton 2:    Set Home\n\rButton 3:    Set Max Limit\n\rButton 4:    Exit Mode 1\n\r");
          blink(1);
        }
        // All the below test if SW3 or SW4 have been clicked in them, choose to make debug_target be main factor because of "3" the spinndle
        if (debug_target == 0) // xstepper mottor.
        {
          x_place = x_place + debug_step(Xstep, Xdir);
        }
        if (debug_target == 1) // ystepper mottor.
        {
          y_place = y_place + debug_step(Ystep, Ydir);
        }
        if (debug_target == 2) // zstepper mottor.
        {
          z_place = z_place + debug_step(Zstep, Zdir);
        }
        if (debug_target == 3) // Spindle mottor.
        {
          if (!gpio_get(SW3))
          {
            pwm_set_gpio_level(Spindle, 0 * 0);
          }
          else if (!gpio_get(SW4))
          {
            pwm_set_gpio_level(Spindle, 127);
          }
        }
      }
      if (!gpio_get(SW1))
      {
        debug = 1;
        term_cls();
        term_move_to(1, 1);
        printf("Button 1:    Options\n\rButton 2:    Change Motor\n\rButton 3:    Move Negative\n\rButton 4:    Move Positive\n\n\r");
        blink(1);
      }
      else if (!gpio_get(SW2))
      { // set 0 0 0
        x_place = 0;
        y_place = 0;
        z_place = 0;
        debug = 1;
        term_cls();
        term_move_to(1, 1);
        printf("Button 1:    Options\n\rButton 2:    Change Motor\n\rButton 3:    Move Negative\n\rButton 4:    Move Positive\n\n\r");
        blink(2);
      }
      else if (!gpio_get(SW3))
      { // set lims
        x_max = x_place;
        y_max = y_place;
        z_max = z_place;
        debug = 1;
        term_cls();
        term_move_to(1, 1);
        printf("Button 1:    Options\n\rButton 2:    Change Motor\n\rButton 3:    Move Negative\n\rButton 4:    Move Positive\n\n\r");
        blink(3);
      }
      else if (!gpio_get(SW4))
      {
        codeType = 0;
        term_cls();
        term_move_to(1, 1);
        printf("Button 1:    Debug Mode\n\rButton 2:    Terminal\n\rButton 3:    Currently not in use\n\rButton 4:    Currently not in use\n\r");

        blink(4);
      }
    }
    while (codeType == 2) // Terminal
    {
      x_move = 0;
      y_move = 0;
      z_move = 0;
      if (write == 1) // Prints terminal
      {
        indexx = 0;
        term_set_color(clrYellow, clrBlack);
        term_move_to(30, 4);
        printf("goto x y z Moves milling arm to (x,y,z) position");
        term_move_to(30, 5);
        printf("move x y z Moves (x,y,z) from position          ");
        term_move_to(30, 6);
        printf("codetype n Changes between code modes           ");
        term_move_to(30, 7);
        printf("circle r   Draw circle with (r) radius from left");
        term_move_to(30, 8);
        printf("wasd       Enables WASD control through terminal");
        term_move_to(30, 9);
        printf("home       Set current point to home            ");
        term_move_to(30, 10);
        printf("set lim    Set maximum limit ot current (xyz)   ");
        term_move_to(30, 11);
        printf("gcode      Enables Gcode mode ready to be pasted");
        term_move_to(30, 12);
        printf("units      Change all units between steps and mm");
        term_set_color(clrBlack, clrYellow);
        term_draw_box(0, 3, 27, 13, "+-------[ Milling ]-------+");
        term_draw_box(29, 3, 78, 13, "+------------------[ Function ]------------------+");
        term_draw_box(29, 3, 40, 13, "");
        write = 2;
      }
      if (write == 2) // Prints info
      {
        term_set_color(clrYellow, clrBlack);
        term_move_to(3, 4);
        printf("                        ");
        term_move_to(3, 4);
        if (scale == 1)
        {
          printf("Units: step");
        }
        else
        {
          printf("Units: mm");
        }
        term_move_to(3, 6);
        printf("                        ");
        term_move_to(3, 7);
        printf("                        ");
        term_move_to(3, 8);
        printf("                        ");
        term_move_to(3, 6);
        printf("X Position: %d", x_place);
        term_move_to(3, 7);
        printf("Y Position: %d", y_place);
        term_move_to(3, 8);
        printf("Z Position: %d", z_place);

        term_move_to(3, 10);
        printf("Current command: ");

        term_move_to(1, 15);
        printf(">> ");
        write = 0;
      }
      int ch = getchar_timeout_us(0);                     // gets key pressed
      if ((ch == '\r') || (ch == '\n') || (indexx == 99)) // runs command.
      {
        buffer[indexx] = 0;
        indexx = 0;
        term_move_to(4, 15);
        term_erase_line();
        term_move_to(3, 11);
        volatile int valx = 0;
        volatile int valy = 0;
        volatile int valz = 0;
        volatile int val = 0;
        volatile float gcodex = 0;
        volatile float gcodey = 0;
        if (gcode == 1)
        {
          if (sscanf(buffer, "G1 X%f Y%f F100.00", &gcodex, &gcodey) > 0)
          {
            x_move = x_max / 2 - gcodex * 1000 - x_place;
            y_move = y_max / 2 + gcodey * 1000 - y_place;
          }
          else if (strcmp(buffer, "M300 S30.00 (pen down)") == 0)
          {
            pwm_set_gpio_level(Spindle, 127);
            z_move = -10000;
          }
          else if (strcmp(buffer, "M300 S50.00 (pen up)") == 0)
          {
            z_move = 10000;
            pwm_set_gpio_level(Spindle, 0);
          }
          else if (strcmp(buffer, "(end of print job)") == 0)
          {
            gcode = 0;
          }
        }
        else if (sscanf(buffer, "goto %d %d %d", &valx, &valy, &valz) > 0) // goto command makes it move to the set step possition.
        {
          x_move = valx * scale - x_place;
          y_move = valy * scale - y_place;
          z_move = valz * scale - z_place;

          printf("Moved to position       ");
          term_move_to(3, 12);
          printf("                        ");
          term_move_to(3, 12);
          printf("[%d, %d, %d]", x_move, y_move, z_move);
        }
        else if (sscanf(buffer, "move %d %d %d", &valx, &valy, &valz) > 0) // move the set amount from current possition.
        {
          x_move = valx * scale;
          y_move = valy * scale;
          z_move = valz * scale;
          printf("Moving                  ");
          term_move_to(3, 12);
          printf("                        ");
          term_move_to(3, 12);
          printf("[%d, %d, %d]", x_move, y_move, z_move);
        }
        else if (sscanf(buffer, "codetype %d", &val) > 0) // changes the codetype.
        {
          if ((val >= 0) && (val <= 4)) // if legal value
          {
            term_cls();
            write = 1;
            codeType = val;
          }
          else
          {
            printf("Invalid codeType\n\r");
            term_move_to(3, 12);
            printf("                         \n\r");
          }
        }
        else if (sscanf(buffer, "circle %d", &val) > 0) // draws circle radius val.
        {
          printf("Draw circle             ");
          term_move_to(3, 12);
          printf("                        ");
          term_move_to(3, 12);
          printf("[r=%d]", val * scale);
          draw_circle(val * scale, x_place, y_place, z_place, x_max, y_max, z_max);
        }
        else if (strcmp(buffer, "wasd") == 0) // wasdmode.
        {
          printf("wasd mode activated     ");
          term_move_to(3, 12);
          printf("                        ");
          term_set_color(clrYellow, clrBlack);
          term_move_to(30, 4);
          printf("                                                ");
          term_move_to(30, 5);
          printf(" W and S   Move milling arm Forward and Backward");
          term_move_to(30, 6);
          printf(" W and S   Move milling arm Left and Right      ");
          term_move_to(30, 7);
          printf(" W and S   Move milling arm Up and Down         ");
          term_move_to(30, 8);
          printf(" Space Bar Toggle spindle                       ");
          term_move_to(30, 9);
          printf("    H      Set Home position                    ");
          term_move_to(30, 10);
          printf("    L      Set max limit to current position    ");
          term_move_to(30, 11);
          printf("    X      Exit current mode                    ");
          term_move_to(30, 12);
          printf("                                                ");
          while (ch != 'x')
          {
            term_move_to(3, 15);
            int ch = getchar_timeout_us(0);
            if (ch == 'q')
            {
              printf(" Down     ");
              z_move = 100;
            }
            else if (ch == 'w')
            {
              printf(" Forward  ");
              y_move = 300;
            }
            else if (ch == 'e')
            {
              printf(" Up       ");
              z_move = -100;
            }
            else if (ch == 'a')
            {
              printf(" Left     ");
              x_move = -300;
            }
            else if (ch == 's')
            {
              printf(" Backwards");
              y_move = -300;
            }
            else if (ch == 'd')
            {
              printf(" Right    ");
              x_move = 300;
            }
            else if (ch == ' ')
            {
              if (spindle_on == 0)
              {
                printf(" Spinning");
                pwm_set_gpio_level(Spindle, 127);
                spindle_on = 1;
              }
              else
              {
                printf(" Stoped   ");
                pwm_set_gpio_level(Spindle, 0);
                spindle_on = 0;
              }
              sleep_ms(2);
            }
            else if (ch == 'x')
            {
              printf("          ");
              term_move_to(3, 11);
              printf("wasd mode deactivated   ");
              term_move_to(3, 12);
              printf("                        ");
              write = 1;
              break;
            }
            else if (ch == 'l')
            {
              printf(" Set Limit");
              x_max = x_place;
              y_max = y_place;
              z_max = z_place;
            }
            else if (ch == 'h')
            {
              printf(" Set Home ");
              x_place = 0;
              y_place = 0;
              z_place = 0;
            }
            moving_steps(&x_move, &y_move, &z_move, &x_place, &y_place, &z_place, x_max, y_max, z_max, 1);
          }
        }
        else if (strcmp(buffer, "home") == 0) // set home
        {
          x_place = 0;
          y_place = 0;
          z_place = 0;
          printf("Set home to             ");
          term_move_to(3, 12);
          printf("                        ");
          term_move_to(3, 12);
          printf("[%d,%d,%d]\n\r", x_place, y_place, z_place);
        }
        else if (strcmp(buffer, "gcode") == 0) // set home
        {
          printf("Gcode mode activated    ");
          term_move_to(3, 12);
          printf("                        ");
          term_move_to(3, 12);
          printf("Waiting for input");
          z_move = 10000;
          pwm_set_gpio_level(Spindle, 0);
          gcode = 1;
        }
        else if (strcmp(buffer, "set lim") == 0)
        {
          x_max = x_place;
          y_max = y_place;
          z_max = z_place;
          printf("New limits set to       ");
          term_move_to(3, 12);
          printf("                        ");
          term_move_to(3, 12);
          printf("[%d,%d,%d]\n\r", x_max, y_max, z_max);
        }
        else if (strcmp(buffer, "units") == 0)
        {
          if (scale == 1)
          {
            scale = 1595;
            printf("Units set to mm         ");
            term_move_to(3, 12);
            printf("                        ");
          }
          else
          {
            scale = 1;
            printf("Units set to step       ");
            term_move_to(3, 12);
            printf("                        ");
          }
        }
        else
        {
          printf("Nothing run             \n\r");
          term_move_to(3, 12);
          printf("                        \n\r");
        }

        /////////////////////////////////////////////////////////////
        moving_steps(&x_move, &y_move, &z_move, &x_place, &y_place, &z_place, x_max, y_max, z_max, 0);
        if (write == 0)
        {
          write = 2;
        }
        /////////////////////////////////////////////////////////////
      }
      else if (ch == '\177') // backspace.
      {
        if (indexx != 0)
        {
          buffer[indexx] = ch;
          indexx--;
          printf("%c", ch);
        }
      }
      else if (ch == '\f') // not too sure about this think it is the clear terminal command.
      {
        buffer[indexx] = 0;
        indexx = 0;
        printf("%c", ch);
        write = 1;
      }
      else if (ch != -1) // if any other char print it and move to next buffer slot.
      {
        buffer[indexx] = ch;
        indexx++;
        printf("%c", ch);
      }
    }
    while (codeType == 3) // Runs through list of cordanates.
    {
      x_move = xlist[cur_step] - x_place;
      y_move = ylist[cur_step] - y_place;
      z_move = zlist[cur_step];
      cur_step++;
      moving_steps(&x_move, &y_move, &z_move, &x_place, &y_place, &z_place, x_max, y_max, z_max, 0);
    }
    while (codeType == 4) // testing area ATM, currently draws circles.
    {
      for (int size = 100; size <= 2000; size = size + 100)
      {
        for (int i = 0; i <= (4 * size); i++)
        {
          if ((i <= (2 * size)) && (size % 200 != 0))
          {
            y_move = i - y_place;
            z_move = sqrt(size * size - (i - size + size - 100) * (i - size + size - 100)) - z_place;
          }
          else if ((i > (2 * size)) && (size % 200 == 0))
          {
            y_move = 4 * size - i - y_place;
            z_move = 0 - sqrt(size * size - (3 * size - i - size) * (3 * size - i - size)) - z_place - size;
          }
          moving_steps(&x_move, &y_move, &z_move, &x_place, &y_place, &z_place, x_max, y_max, z_max, 0);
        }
      }
    }
  }
}
