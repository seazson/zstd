#ifndef __LINUX_IF_CSM_ENCAPS_H
#define __LINUX_IF_CSM_ENCAPS_H

#include <linux/if_ether.h>     /* for ETH_DATA_LEN */

/* Packet types */
#define ETH_P_CSME		0x889B
#define AF_CSME			27
#define PF_CSME			AF_CSME



struct csme_cmd {
	int noofcommands;
};

/* values for get/set options */
#define CSME_ACK_ENABLE		1
#define CSME_TARGET_RESET	2

struct sockaddr_csme {
	unsigned short	scsme_family;       /* AF_XXX family */
	int		scsme_ifindex;      /* interface index */
	unsigned char	scsme_devmac[6];    /* device MAC address */
	unsigned short	scsme_opcode;       /* opcode */
	unsigned short	scsme_channelid;    /* Channel ID  */
	unsigned short	scsme_flags;
};

#define	CSME_OPCODE_NOOP		0x0000
#define	CSME_OPCODE_CONTROL		0x0001
#define	CSME_OPCODE_RESERVED		0x0002
#define CSME_OPCODE_UNIFIED_DIAGNOSTICS	0x0003
#define CSME_OPCODE_REMOTE_MEDIA	0x0004
#define CSME_OPCODES_NUMBER		5


struct csme_ackopt {
	unsigned char	scsme_devmac[6];	/* Target Device MAC */
	int		scsme_ifindex;		/* interface index */
	unsigned char	ack_suppression;	/* 0 = ACK Packet is required(default) */
						/* 1 = ACK packet must be suppressed */
};

struct csme_resetopt {
	unsigned char	scsme_devmac[6];	/* Target Device MAC */
	unsigned char	scsme_newmac[6];	/* Target Device new MAC */
};
#endif	/* __LINUX_IF_CSM_ENCAPS_H */
