#ifndef _CSM_ENCAPS_H
#define _CSM_ENCAPS_H

#include <asm/byteorder.h>

#define	CSME_CBF_ACK_BIT		0x80
#define	CSME_CBF_ACK_SUPR_BIT		0x40
#define CSME_CBF_LITTLE_END		0x01
#define CSME_CBF_BOOTLOAD_BIT		0x01

#define CMD_CLASS_ETH_BOOT_LDR		0x04
#define CMD_CLASS_CONFIGURATION_DEVICE	0x06

#define CMD_TYPE_MAAS_ASSIGN		0x1B
#define CMD_TYPE_QUERY_READY		0x1E
#define CMD_TYPE_READY			0x20
#define CMD_TYPE_CONFIGURATION_RESPONSE 0x02

/* function code for Command class 0x06 */
#define FCODE_SUPV_CREATE_CHANNEL	0x0010
#define FCODE_SUPV_DESTROY_CHANNEL	0x0011
#define FCODE_CONF_CREATE_PARTICIPANT	0x9312

#define CSME_MAX_CHANNELS	640

struct csme_hdr
{
	u16 opcode;

	u8 seq_number;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u8 endianess:1;
	u8 reserved:5;
	u8 ack_sup:1;
	u8 cr:1;
#elif defined (__BIG_ENDIAN_BITFIELD)
	u8 cr:1;
	u8 ack_sup:1;
	u8 reserved:5;
	u8 endianess:1;
#else
#error	"Please fix <asm/byteorder.h>"
#endif

	u16 channel_nb;
} __attribute__((packed));


struct api_hdr
{
	u8 length;
	u8 index;

	u8 cmd_type;
	u8 cmd_class;

	u16 func_code;
	u16 reserved;
} __attribute__((packed));

struct api_hdr_boot
{
	u8 index;
	u8 length;

	u8 cmd_class;
	u8 cmd_type;

	u16 func_code;
} __attribute__((packed));

struct csme_channel {
	struct csme_target *target;	/* target under which this channel is created */
	unsigned short id;	/* channel ID/ number assigned to this channel */

	struct sk_buff_head tx_queue;
	u8 tx_seq_number[CSME_OPCODES_NUMBER];

	spinlock_t lock;

	struct timer_list timer;
	int retries;

	u8 last_rx_seq_number[CSME_OPCODES_NUMBER];
};

struct csme_stats {
	atomic_t tx_msg;
	atomic_t tx_msg_total_retransmitted;
	atomic_t tx_msg_max_retransmissions;
	atomic_t tx_msg_retransmitted;
	atomic_t tx_msg_err;

	atomic_t rx_ack;
	atomic_t rx_ack_wrongseq;
	atomic_t rx_ack_err;

	atomic_t rx_msg;
	atomic_t rx_msg_repeated;
	atomic_t rx_msg_err;

	atomic_t tx_ack;
	atomic_t tx_ack_err;
};

struct csme_target {
	unsigned char macaddr[6];	/* MAC address of the target */

	/* New Channel Stuff */
	struct csme_channel boot_channel;	/* channel used while bootloading */
	struct csme_channel supvsr_channel;	/* supervisory channel */

	rwlock_t lock;

	struct csme_channel *channel[CSME_MAX_CHANNELS];	/* table of channels in this target */

	int scsme_ifindex;
	unsigned char ack_suppression;

	struct list_head list;

	struct csme_stats stats;
};

#endif /* _CSM_ENCAPS_H */
