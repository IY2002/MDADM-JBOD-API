#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

uint32_t op_reader(uint32_t op){
  return op >> 26;
}

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf) {
  int bytes_read = 0;
  while(bytes_read < len){
    int read_result = read(fd, buf + bytes_read, len - bytes_read);
    if (read_result == -1){
      return false;
    }else{
      bytes_read += read_result;
    }
  }
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
  int bytes_written = 0;
  while(bytes_written < len){
    int write_result = write(fd, buf + bytes_written, len - bytes_written);
    if (write_result == -1){
      return false;
    }else{
      bytes_written += write_result;
    }
  }
  return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {

  uint16_t len;

  uint8_t packet_header[HEADER_LEN];

  nread(cli_sd, HEADER_LEN, packet_header);

  int bytes_read = 0;

  memcpy(&len, &packet_header[bytes_read], sizeof(len));
  bytes_read += sizeof(len);
  len = ntohs(len);

  memcpy(&op, &packet_header[bytes_read], sizeof(op));
  bytes_read += sizeof(op);

  memcpy(&ret, &packet_header[bytes_read], sizeof(ret));
  bytes_read += sizeof(ret);

  if (len > HEADER_LEN)
    nread(cli_sd, JBOD_BLOCK_SIZE, block);

  return true;
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {
  uint16_t len = HEADER_LEN;
  if (op_reader(op) == JBOD_WRITE_BLOCK){
    //send block
    len += JBOD_BLOCK_SIZE;
  }
  
  uint16_t len_network = htons(len);
  uint16_t return_code = htons(0);
  uint32_t op_network = htonl(op);

  uint8_t packet[len];

  int bytes_in_packet = 0;

  memcpy(&packet[bytes_in_packet], &len_network, sizeof(len_network));
  bytes_in_packet += sizeof(len_network);

  memcpy(&packet[bytes_in_packet], &op_network, sizeof(op_network));
  bytes_in_packet += sizeof(op_network);

  memcpy(&packet[bytes_in_packet], &return_code, sizeof(return_code));
  bytes_in_packet += sizeof(return_code);

  if (op_reader(op) == JBOD_WRITE_BLOCK){
    memcpy(&packet[bytes_in_packet], block, JBOD_BLOCK_SIZE);
  }

  return nwrite(cli_sd, len, packet);
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {
  struct sockaddr_in cli_addr;
  cli_addr.sin_family = AF_INET;
  cli_addr.sin_port = htons(port);

  if (inet_aton(ip, &cli_addr.sin_addr) == 0){
    return false;
  }

  cli_sd = socket(PF_INET, SOCK_STREAM, 0);
  if(cli_sd == -1){
    return false;
  }

  if(connect(cli_sd, (const struct sockaddr *) &cli_addr, sizeof(cli_addr)) == -1){
    return false;
  }
  return true;
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
  close(cli_sd);
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {

  // try to send packet
  if (!send_packet(cli_sd, op, block))
    return -1;

  uint16_t recv_return;
  uint32_t recv_op;
  // try to recieve packet
  if (!recv_packet(cli_sd, &recv_op, &recv_return, block))
    return -1;
  return 1;

}
