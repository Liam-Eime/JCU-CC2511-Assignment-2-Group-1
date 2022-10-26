/** \file terminal.h
 *  \defgroup pico_term
 *
 * Header-only code for simple terminal escape codes.
 * For use with printf(), as provided by pico_stdio. 
 *
 * Based on:
 *   https://en.wikipedia.org/wiki/ANSI_escape_code
 *   https://en.wikipedia.org/wiki/ASCII
  */


#ifndef CC2511_TERMINAL_H
#define CC2511_TERMINAL_H

#include <stdio.h>
#include "pico/stdlib.h"

/* Color constants */
#define   clrBlack   30U                /* Black color */
#define   clrRed     31U                /* Red color */
#define   clrGreen   32U                /* Green color */
#define   clrYellow  33U                /* Yellow color */
#define   clrBlue    34U                /* Blue color */
#define   clrMagenta 35U                /* Magenta color */
#define   clrCyan    36U                /* Cyan color */
#define   clrWhite   37U                /* White color */

/* Character sequence constants */
static const char term_data_esc_prefix[]  = { 0x1BU, 0x5BU };
static const char term_data_cls[]         = { 0x32U, 0x4AU };

/*! \brief Clear the terminal.
 *  \ingroup pico_term
 *
 * Clears all content from the screen and returns the cursor to position 1,1.
 */
static inline int term_cls(void) {
  return printf("%s%s", term_data_esc_prefix, term_data_cls);
}

/*! \brief Move cursor to position x,y.
 *  \ingroup pico_term
 *
 * \param x Column number (1-based, left-to-right)
 * \param y Row number (1-based, top-to-bottom)
 */
static inline int term_move_to(unsigned short x, unsigned short y) {
  return printf("%s%d;%dH", term_data_esc_prefix, y, x);
}

/*! \brief Set terminal foreground and background colours.
 *  \ingroup pico_term
 *
 *  Colours remain active until instructed otherwise by
 *  calling this function again.
 * 
 * \param foreground Foreground colour (30-37; see clr* listing)
 * \param background Background colour (30-37; see clr* listing)
 */
static inline int term_set_color(unsigned short foreground, 
  unsigned short background) {
  return printf("%s0;%d;%dm", term_data_esc_prefix, foreground, 
    background + ((background < 40) ? 10 : 0));
}

/*! \brief Erase terminal current line.
 *  \ingroup pico_term
 */
static inline int term_erase_line(void) {
  return printf("%sK", term_data_esc_prefix);
}

/*! \brief Draw a box from a point to point.
 *  \ingroup pico_term
 *
 * \param x1 First x point
 * \param y1 First y point
 * \param x2 Second x point
 * \param y2 Second y point
 */
static inline void term_draw_box(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2, char *title) {
  term_move_to(x1,y1);
  printf("%s", title);
  for (int i = sizeof(title); i < x2-1;){
    printf(" ");
    i++;
  }
  
  for (int i=1; i < y2-2;){
    term_move_to(x1, y1 + i);
    printf(" ");
    term_move_to(x2, y1 + i);
    printf(" ");
    i++;
  }
  
  term_move_to(x1,y2);
  for (int i=0;i<x2;){
    printf(" ");
    i++;
  }

}

#endif //  CC2511_TERMINAL_H

