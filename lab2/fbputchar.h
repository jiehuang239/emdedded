#ifndef _FBPUTCHAR_H
#  define _FBPUTCHAR_H

#define FBOPEN_DEV -1          /* Couldn't open the device */
#define FBOPEN_FSCREENINFO -2  /* Couldn't read the fixed info */
#define FBOPEN_VSCREENINFO -3  /* Couldn't read the variable info */
#define FBOPEN_MMAP -4         /* Couldn't mmap the framebuffer memory */
#define FBOPEN_BPP -5          /* Unexpected bits-per-pixel */

struct color{
  char R;
  char G;
  char B;
  char notused;
};

extern struct color darkBlue;
extern struct color lightBlue;
extern struct color darkgrey;
extern struct color grey;
extern struct color greyBlack;
extern struct color *FONTGLOBAL;
extern struct color *BACKGROUNDGLOBAL;




extern int fbopen(void);
extern void fbputchar(char, int, int, struct color, struct color);
extern void fbputs(const char *, int, int, struct color);
extern void scrolldown(int row_h, int row_t, struct color background);
extern void fbclear(int row_h, int row_t, struct color);
extern void fbclearall();
extern void initScreen();
extern void drawline(int row,int col_h,int col_t);
extern void invert(int row, int col);

#endif
