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

int count = 0;
/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

/* keyboard numnber --> ASCII
 * 1 - 9, 0, ENTER, ESC, BACKSPACE, TAB, SPACE,
 * -, =, [, ], \, NOT_FOUND, ;, ', `, ,, ., /
 */
static char keyboard_table[] = {
  0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x00, 0x00, 0x00, 0x09, 0x20,
  0x2d, 0x3d, 0x5b, 0x5d, 0x5c, 0x00, 0x3b, 0x27, 0x60, 0x2c, 0x2e, 0x2f
};

/* shift_keyboard numnber --> ASCII
 * ! - ), ENTER, ESC, BACKSPACE, TAB, SPACE,
 * _, +, {, }, |, NOT_FOUND, :, ", ~, <, >, ?
 */
static char shift_keyboard_table[] = {
  0x21, 0x40, 0x23, 0x24, 0x25, 0x5e, 0x26, 0x2a, 0x28, 0x29, 0x00, 0x00, 0x00, 0x09, 0x20,
  0x5f, 0x2b, 0x7b, 0x7d, 0x7c, 0x00, 0x3a, 0x22, 0x7e, 0x3c, 0x3e, 0x3f
};


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
      return (char)(packet.keycode[0] + 61);
    else
      return (char)(shift_keyboard_table[packet.keycode[0] - 0x1e]);

  } else {
    if (packet.keycode[0] >= 0x04 && packet.keycode[0] <= 0x1d)
      return (char)(packet.keycode[0] + 93);
    else
      return (char)(keyboard_table[packet.keycode[0] - 0x1e]);
  }
  return '.';
}


void delete_word(int* count, char* buffer)
{
  fbputchar(' ', 22 + (*count)/64, (*count)%64);
  (*count)--;
  buffer[*count] = '\0';
  // fbputchar(' ', 22 + (*count)/22, (*count)%22);
  fbputchar('|', 22 + (*count)/64, (*count)%64);
}

void add_word(int* count, char* buffer, char word) 
{
  fbputchar(word, 22 + (*count)/64, (*count)%64);
  buffer[(*count)++] = word;
  buffer[*count] = '\0';
  fbputchar('|', 22 + (*count)/64, (*count)%64);
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
    count = 0;
    char buffer[BUFFER_SIZE];
    buffer[BUFFER_SIZE - 1] = '\0';
    for (;;) {
      libusb_interrupt_transfer(keyboard, endpoint_address,
            (unsigned char *) &packet, sizeof(packet),
            &transferred, 0);
      if (transferred == sizeof(packet)) {
	
	if (packet.keycode[0] == 0x00)
	  continue;
        else if (packet.keycode[0] == 0x2a) { // delete
          if (count == 0)
            continue;
          else
            delete_word(&count, &buffer[0]);
        } else if (packet.keycode[0] == 0x28) { // enter
          break;
        } else if (packet.keycode[0] == 0x29) { // esc
          exit = 1;
          break;
        } else {
          if (count >=  BUFFER_SIZE - 1)
            continue;
          char tmp = interpret_key(packet);
          add_word(&count, &buffer[0], tmp);
        }

      }
    }
    if (exit)
      break;
    fbclear(22, 23, 0, 63);
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
    printf("%s", recvBuf);
    fbputs(recvBuf, 0, 0);
  }

  return NULL;
}
