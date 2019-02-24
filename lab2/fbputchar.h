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
struct color darkBlue={
	.R=1,
	.G=80,
	.B=227,
	.notused=0
 
};
struct color lightBlue={
	.R=83,
	.G=141,
	.B=231
};
struct color darkgrey={
	.R=185,
	.G=182,
	.B=177,
	.notused=0
};
struct color grey={
	.R=208,
	.G=214,
	.B=217,
	.notused=0
};
struct color greyBlack={
	.R=39,
	.G=32,
	.B=29,
	.notused=0
};
struct color *FONTGLOBAL=&greyBlack;
struct color *BACKGROUNDGLOBAL=&grey;
extern int fbopen(void);
extern void fbputchar(char, int, int);
extern void fbputs(const char *, int, int);
extern void scrolldown(int row_h, int row_t);
extern void fbclear(int row_h, int row_t, int col_h, int col_t);
extern void fbclearall();
extern void initScreen();
extern void drawline(int row,int col_h,int col_t);
extern void invert(int row, int col);

#endif
