/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Jie Huang/jh4000, Kaige Zhang/kz2325
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* micro36.ee.columbia.edu */
#define SERVER_HOST "128.59.148.182"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128


typedef struct {
  int rev_row;
  int rev_limit;
  int sen_row;
  int sen_limit;
} screen_info;

static screen_info info = {
  .rev_row = REC_LINE + 1,
  .rev_limit = SEN_LINE - 1,
  .sen_row = SEN_LINE + 1,
  .sen_limit = INPUT_LINE - 1
};


/* count for recording position of input string */
static int count = 0;
/* cursor_count for recording position of cursor */
static int cursor_count = 0;

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

/* keyboard numnber --> ASCII
 * 1 - 9, 0, ENTER, ESC, BACKSPACE, TAB, SPACE,
 * -, =, [, ], \, NOT_FOUND, ;, ', `, ,, ., /, CAP
 */
static char keyboard_table[] = {
  0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x00, 0x00, 0x00, 0x00, 0x20,
  0x2d, 0x3d, 0x5b, 0x5d, 0x5c, 0x00, 0x3b, 0x27, 0x60, 0x2c, 0x2e, 0x2f, 0x00
};

/* shift_keyboard numnber --> ASCII
 * ! - ), ENTER, ESC, BACKSPACE, TAB, SPACE,
 * _, +, {, }, |, NOT_FOUND, :, ", ~, <, >, ?, CAP
 */
static char shift_keyboard_table[] = {
  0x21, 0x40, 0x23, 0x24, 0x25, 0x5e, 0x26, 0x2a, 0x28, 0x29, 0x00, 0x00, 0x00, 0x00, 0x20,
  0x5f, 0x2b, 0x7b, 0x7d, 0x7c, 0x00, 0x3a, 0x22, 0x7e, 0x3c, 0x3e, 0x3f, 0x00
};


int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
pthread_t cursor_thread;
void *network_thread_f(void *);

char interpret_key(struct usb_keyboard_packet packet, int index);
void delete_word(char* buffer);
void add_word(char* buffer, char word);
void interpret_arrow(char* buffer,unsigned char key);

int main()
{
  int err;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  // char keystate[12];

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  fbclearall();

  initScreen();
  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);


  for (;;) {
    int exit = 0;
    char buffer[BUFFER_SIZE];
    char prev = 0x00;

    count = 0;
    cursor_count = 0;
    buffer[0] = '\0';

    for (;;) {
      libusb_interrupt_transfer(keyboard, endpoint_address,
            (unsigned char *) &packet, sizeof(packet),
            &transferred, 0);
      
      if (transferred == sizeof(packet)) {
        if (packet.keycode[0] < 0x04) {
          prev = 0x00;
          continue;
	       } else if (packet.keycode[0] == 0x2b) { /* replace TAB for 2 spaces */
          if (count >= BUFFER_SIZE - 1)
	          continue;
	        add_word(buffer, ' ');
	        add_word(buffer, ' ');  
        } else if (packet.keycode[0] >= 0x4f && packet.keycode[0] <= 0x52) { /* move cursor */
	        interpret_arrow(buffer, packet.keycode[0]); 
        } else if (packet.keycode[0] == 0x2a) { /* DELETE handler */
          if (count == 0)
            continue;
          else
            delete_word(buffer);
        } else if (packet.keycode[0] == 0x28) { /* ENTER handler */
          break;
        } else if (packet.keycode[0] == 0x29) { /* ESC handler */
          exit = 1;
          break;
        } else if (packet.keycode[1] == 0x00) { /* single keyboard pressed */
          if (count >=  BUFFER_SIZE - 1 || packet.keycode[0] == prev)
            continue;
          char tmp = interpret_key(packet, 0);
          if (tmp == 0x00)
	          continue;
          add_word(buffer, tmp);
	        prev = packet.keycode[0];
        } else { /* double keyboard pressed and avoid strange situation */
	        if (count >=  BUFFER_SIZE - 1)
            continue;
          int index = 0;
          if (prev == packet.keycode[0])
	          index = 1;
          char tmp = interpret_key(packet, index);
          if (tmp == 0x00)
	          continue;
          add_word(buffer, tmp); 
          prev = packet.keycode[index];
	      }
      }
    }
    if (exit)
      break;
    fbclear(INPUT_LINE + 1, INPUT_LINE + 2, *BACKGROUNDGLOBAL);
    if (count == 0)
      continue;
    int n = write(sockfd, &buffer, count - 1);
    if (n <= 0)
      printf("Sending packet failed\n");


    while (info.sen_limit - info.sen_row < count/64 + 1) {  /* sent region scroll down */
      scrolldown(info.rev_limit + 2, info.sen_limit, *BACKGROUNDGLOBAL);
      info.sen_row--;
    }
    fbputs(buffer, info.sen_row, 0, *BACKGROUNDGLOBAL);
    if (count < 65)
      info.sen_row++;
    else
      info.sen_row += 2;

  }


  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}


void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  int n;
  /* Receive data */
  while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';

    // printf("len: %d, %s\n", n, recvBuf);

    while (info.rev_limit - info.rev_row < count/64 + 1) { /* receive region scroll down */
      scrolldown(REC_LINE + 1, info.rev_limit, *BACKGROUNDGLOBAL);
      info.rev_row--;
    }
    fbputs(recvBuf, info.rev_row, 0, *BACKGROUNDGLOBAL);
    if (n < 65)
      info.rev_row++;
    else
      info.rev_row += 2;
  }
  return NULL;
}

char interpret_key(struct usb_keyboard_packet packet, int index)
{
  int shift = 0;
  if (packet.modifiers & 0x22)
    shift = 1;

  if (shift) {
    if (packet.keycode[index] >= 0x04 && packet.keycode[index] <= 0x1d)
      return (char)(packet.keycode[index] + 61);
    else if (packet.keycode[index] <= 0x39)
      return (char)(shift_keyboard_table[packet.keycode[index] - 0x1e]);

  } else {
    if (packet.keycode[index] >= 0x04 && packet.keycode[index] <= 0x1d)
      return (char)(packet.keycode[index] + 93);
    else if (packet.keycode[index] <= 0x39)
      return (char)(keyboard_table[packet.keycode[index] - 0x1e]);
  }
  return 0x00;
}

/* delete word indicated by cursor_count and refresh framebuffer */
void delete_word(char* buffer)
{
  char *curr = &buffer[cursor_count - 1];
  char *end = &buffer[count];

  while (curr != end) {
    *curr = *(curr + 1);
    curr++;
  }
  fbclear(INPUT_LINE + 1, INPUT_LINE + 2, *BACKGROUNDGLOBAL);
  fbputs(buffer, INPUT_LINE + 1, 0, *BACKGROUNDGLOBAL);
  count--;
  cursor_count--;
  invert(INPUT_LINE + 1 + cursor_count/64, cursor_count%64);

}

/* add word indicated by cursor_count and refresh framebuffer */
void add_word(char* buffer, char word) 
{

  char *curr = &buffer[cursor_count];
  char *end = &buffer[count + 1];

  while (curr != end) {
    *end = *(end - 1);
    end--;
  }
  *curr = word;
  fbclear(INPUT_LINE + 1, INPUT_LINE + 2, *BACKGROUNDGLOBAL);
  fbputs(buffer, INPUT_LINE + 1, 0, *BACKGROUNDGLOBAL);
  count++;
  cursor_count++;
  invert(INPUT_LINE + 1 + cursor_count/64, cursor_count%64);

}

/* cursor moving handler */
void interpret_arrow(char* buffer, unsigned char key)
{
  switch (key)
  {
    case 0x4f:
      /* cursor moving right */
      if (cursor_count == count)
        break;
      else {
        invert(INPUT_LINE + 1 + cursor_count/64, cursor_count%64);
        cursor_count++;
        invert(INPUT_LINE + 1 + cursor_count/64, cursor_count%64); 
      }        
      break;
    case 0x50:
      /* cursor moving left */
      if (cursor_count == 0)
        break;
      else {
        invert(INPUT_LINE + 1 + cursor_count/64, cursor_count%64);
        cursor_count--;
        invert(INPUT_LINE + 1 + cursor_count/64, cursor_count%64); 
      }
      break;
    case 0x51:
      /* Down case not handled */
      break;
    case 0x52:
      /* Up case not handled */
      break;
    default:
      break;
  }
}

