/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Please Changeto Yourname (pcy2301)
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

#define UNDERLINE   95
#define SPACE       32

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);

char interpret_key(struct usb_keyboard_packet packet)
{
  int shift = 0;
  if (packet.modifiers && 0x22)
    shift = 1;

  if (shift) {
    if (packet.keycode[0] >= 0x04 && packet.keycode[0] <= 0x1d)
      return char(packet.keycode[0] + 61);
    else if (packet.keycode[0] >= 0x1e && packet.keycode[0] <= 0x27)
      return char(shift_keyboard_table[packet.keycode[0] - 0x1e];

  } else {
    if (packet.keycode[0] >= 0x04 && packet.keycode[0] <= 0x1d)
      return char(packet.keycode[0] + 93);
    else if (packet.keycode[0] >= 0x1e && packet.keycode[0] <= 0x27)
      return char(keyboard_table[packet.keycode[0] - 0x1e];
  }
  return '';
}


int main()
{
  int err;

  struct sockaddr_in serv_addr;

  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }

  fbclearall();

  /* Draw rows of asterisks across the top and bottom of the screen */
  /*for (col = 0 ; col < 64 ; col++) {
    fbputchar('*', 0, col);
    fbputchar('*', 23, col);
  }

  fbputchar('*', 5, 5);
  fbputchar('*', 4, 5);
  fbputs("Hello CSEE 4840 World!", 4, 10);
  scrolldown(3, 4);
  */
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

  /* Look for and handle keypresses */
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == sizeof(packet)) {
      sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0],
	      packet.keycode[1]);
      printf("%s\n", keystate);
      char tmp = interpret_key(packet);
      fbputchar(tmp, 6, 10);
      fbputs(keystate, 6, 0);
      if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	break;
      }
    }
  }

/*
  for (;;) {
    int exit = 0;
    for (;;) {
      libusb_interrupt_transfer(keyboard, endpoint_address,
            (unsigned char *) &packet, sizeof(packet),
            &transferred, 0);
      if (transferred == sizeof(packet)) {


        if (packet.keycode[0] == 0x111) { 

        } else if (packet.keycode[0] == 0x111) { 

        } else if (packet.keycode[0] == 0x29) { 
          exit = 1;
          break;
        } else { 

        }
    }
    if (exit)
      break;
  }

*/
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
    printf("%s", recvBuf);
    fbputs(recvBuf, 0, 0);
  }

  return NULL;
}

/* keyboard numnber --> ASCII
 * 1 - 9, 0, ENTER, ESC, BACKSPACE, TAB, SPACE,
 * -, +, [, ], \, NOT_FOUND, ;, ', `, ,, ., /
 */
static unsigned char keyboard_table[] = {
  0x31, 0x32, 0x32, 0x33, 0x34, 0x35, 0x35, 0x36, 0x37, 0x30, 0x00, 0x00, 0x00, 0x09, 0x20,
  0x2d, 0x3d, 0x5b, 0x5d, 0x5c, 0x00, 0x3b, 0x27, 0x60, 0x2c, 0x2e, 0x2f
}

/* shift_keyboard numnber --> ASCII
 * 1 - 9, 0, ENTER, ESC, BACKSPACE, TAB, SPACE,
 * -, +, [, ], \, NOT_FOUND, ;, ', `, ,, ., /
 */
static unsigned char shift_keyboard_table[] = {
  0x21, 0x40, 0x23, 0x24, 0x25, 0x5e, 0x26, 0x2a, 0x28, 0x29, 0x00, 0x00, 0x00, 0x09, 0x20,
  0x5f, 0x2b, 0x7b, 0x7d, 0x7c, 0x00, 0x3a, 0x22, 0x7e, 0x3c, 0x3e, 0x3f
}

