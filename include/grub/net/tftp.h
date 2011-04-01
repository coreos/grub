#ifndef GRUB_NET_TFTP_HEADER
#define GRUB_NET_TFTP_HEADER	1

#include <grub/misc.h>
#include <grub/net/ethernet.h>
#include <grub/net/udp.h>

/* IP port for the MTFTP server used for Intel's PXE */
#define MTFTP_SERVER_PORT 75
#define MTFTP_CLIENT_PORT 76

#define TFTP_DEFAULTSIZE_PACKET 512
#define TFTP_MAX_PACKET 1432

/* IP port for the TFTP server */
#define TFTP_SERVER_PORT 69


/* We define these based on what's in arpa/tftp.h. We just like our
 *  names better, cause they're clearer */
#define TFTP_RRQ 1
#define TFTP_WRQ 2
#define TFTP_DATA 3
#define TFTP_ACK 4
#define TFTP_ERROR 5
#define TFTP_OACK 6

#define TFTP_CODE_EOF 1
#define TFTP_CODE_MORE 2
#define TFTP_CODE_ERROR 3
#define TFTP_CODE_BOOT 4
#define TFTP_CODE_CFG 5

#define TFTP_EUNDEF 0                   /* not defined */
#define TFTP_ENOTFOUND 1                /* file not found */
#define TFTP_EACCESS 2                  /* access violation */
#define TFTP_ENOSPACE 3                 /* disk full or allocation exceeded */
#define TFTP_EBADOP 4                   /* illegal TFTP operation */
#define TFTP_EBADID 5                   /* unknown transfer ID */
#define TFTP_EEXISTS 6                  /* file already exists */
#define TFTP_ENOUSER 7                  /* no such user */

 /*  * own here because this is cleaner, and maps to the same data layout.
 *   */

typedef struct tftp_data
  {
    int file_size;
    int block;
  } *tftp_data_t;


struct tftphdr {
  grub_uint16_t opcode;
  union {
    grub_int8_t rrq[TFTP_DEFAULTSIZE_PACKET];
    struct {
      grub_uint16_t block;
      grub_int8_t download[TFTP_MAX_PACKET];
    } data;
    struct {
      grub_uint16_t block;
    } ack;
    struct {
      grub_uint16_t errcode;
      grub_int8_t errmsg[TFTP_DEFAULTSIZE_PACKET];
    } err;
    struct {
      grub_int8_t data[TFTP_DEFAULTSIZE_PACKET+2];
    } oack;
  } u;
} __attribute__ ((packed)) ;

#endif 
