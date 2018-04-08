/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		CSM_ENCAPS - implements csm_encaps protocol socket.
 *
 *
 */

#include <linux/autoconf.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <asm/ioctls.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <asm/unaligned.h>

#include "if_csmencaps.h"
#include <csmencaps.h>

#define CSME_VERSION	"0.10.1vapi-cvs"


static inline struct csme_sock *csme_sk(struct sock *sk)
{
	return (struct csme_sock *)sk;
}

#define SOL_CSME  269

#define SUPVSR_CH	0xffff
#define BOOT_CH		0x0000


#define CSME_HDR_LEN		(sizeof(struct csme_hdr))		/* 6 bytes */
#define API_HDR_BOOT_LEN	(sizeof(struct api_hdr_boot))		/* 6 bytes */
#define API_HDR_LEN		(sizeof(struct api_hdr))		/* 8 bytes */

#define macAddrEqual(t,f) ((((u32 *)(t))[0] == ((u32 *)(f))[0]) && (((u16 *)(t))[2] == ((u16 *)(f))[2]))

#define printMacAddr(a)		printk(KERN_INFO "mac address-->%#x:%#x:%#x:%#x:%#x:%#x\n",(a)[0],(a)[1],(a)[2],(a)[3],(a)[4],(a)[5])

#define TIMEOUT_MS	200				/* timeout in ms */
#define TIMEOUT		((TIMEOUT_MS * HZ)/1000)	/* timeout in jiffies */
#define RETRIES		(5000 / TIMEOUT_MS)		/* retry for 5 seconds */

unsigned char bcastaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

LIST_HEAD(csme_target_list);	/* all target list */

HLIST_HEAD(csme_sklist);

static DEFINE_RWLOCK(csme_sklist_lock);

atomic_t csme_socks_nr;

static rwlock_t csme_lock;

static void handle_channel_timeout(unsigned long data);

static void channel_reset(struct csme_channel *channel);

/* Private csmencaps socket structures. */
struct csme_sock {
	struct sock sk;
};

/**
 * dump_csme_hdr -
 *
 *
 */
void dump_csme_hdr(struct csme_hdr *hdr)
{
	printk(KERN_INFO "op code:    %#x\n", ntohs(hdr->opcode));
	printk(KERN_INFO "seq number: %#x\n", hdr->seq_number);
	printk(KERN_INFO "endianess:  %#x\n", hdr->endianess);
	printk(KERN_INFO "c/r:        %#x\n", hdr->cr);
	printk(KERN_INFO "ack sup:    %#x\n", hdr->ack_sup);
	printk(KERN_INFO "reserved:   %#x\n", hdr->reserved);
	printk(KERN_INFO "ch number:  %#x\n", ntohs(hdr->channel_nb));
}

/**
 * dump_api_hdr -
 *
 *
 */
void dump_api_hdr(struct api_hdr *hdr)
{
	printk(KERN_INFO "index:    %#x\n", hdr->index);
	printk(KERN_INFO "length:   %#x\n", hdr->length);
	printk(KERN_INFO "cmdclass: %#x\n", hdr->cmd_class);
	printk(KERN_INFO "cmdtype:  %#x\n", hdr->cmd_type);
	printk(KERN_INFO "funccode: %#x\n", hdr->func_code);
}

#ifdef CONFIG_PROC_FS
static int csme_target_read_proc(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
{
	struct csme_target *target = (struct csme_target *) data;
	int len = 0;
	char local_buffer[1024];

	len += sprintf(local_buffer, "tx_msg:                      %d\n", atomic_read(&target->stats.tx_msg));
	len += sprintf(local_buffer + len, "tx_msg_total_retransmitted:  %d\n", atomic_read(&target->stats.tx_msg_total_retransmitted));
	len += sprintf(local_buffer + len, "tx_msg_max_restransmissions: %d\n", atomic_read(&target->stats.tx_msg_max_retransmissions));
	len += sprintf(local_buffer + len, "tx_msg_retransmitted:        %d\n", atomic_read(&target->stats.tx_msg_retransmitted));
	len += sprintf(local_buffer + len, "tx_msg_err:                  %d\n", atomic_read(&target->stats.tx_msg_err));

	len += sprintf(local_buffer + len, "rx_ack:                      %d\n", atomic_read(&target->stats.rx_ack));
	len += sprintf(local_buffer + len, "rx_ack_wrongseq:             %d\n", atomic_read(&target->stats.rx_ack_wrongseq));
	len += sprintf(local_buffer + len, "rx_ack_err:                  %d\n", atomic_read(&target->stats.rx_ack_err));
	
	len += sprintf(local_buffer + len, "rx_msg:                      %d\n", atomic_read(&target->stats.rx_msg));
	len += sprintf(local_buffer + len, "rx_msg_repeated:             %d\n", atomic_read(&target->stats.rx_msg_repeated));
	len += sprintf(local_buffer + len, "rx_msg_err:                  %d\n", atomic_read(&target->stats.rx_msg_err));

	len += sprintf(local_buffer + len, "tx_ack:                      %d\n", atomic_read(&target->stats.tx_ack));
	len += sprintf(local_buffer + len, "tx_ack_err:                  %d\n", atomic_read(&target->stats.tx_ack_err));

	if (offset < len) {
		len -= offset;
		if (len >= length) {
			len = length;
		}
		if (len > 0) {
			memcpy( buffer, local_buffer + offset, len);
		} else {
			len = 0;
		}
	} else {
		len = 0;
	}
	return len;
}

static void target_create_proc(struct csme_target *target)
{
	char s[20 + ETH_ALEN * 3];

	snprintf(s, 20 + ETH_ALEN * 3, "net/csmencaps/%02X:%02X:%02X:%02X:%02X:%02X", target->macaddr[0], target->macaddr[1], target->macaddr[2],
							target->macaddr[3], target->macaddr[4], target->macaddr[5]);

	if (!create_proc_read_entry(s, 0, 0, csme_target_read_proc, target)) {
		printk (KERN_ERR "csme: failed to create %s proc entry\n", s);
	}
}

static void target_remove_proc(struct csme_target *target)
{
	char s[20 + ETH_ALEN * 3];

	snprintf(s, 20 + ETH_ALEN * 3, "net/csmencaps/%02X:%02X:%02X:%02X:%02X:%02X", target->macaddr[0], target->macaddr[1], target->macaddr[2],
							target->macaddr[3], target->macaddr[4], target->macaddr[5]);

	remove_proc_entry(s, NULL);
}

static int csme_socket_read_proc(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
{
	off_t pos = 0;
	off_t begin = 0;
	int len = 0;
	struct sock *sk;
	struct hlist_node *node;
//	struct csme_sock *po;

	len += sprintf(buffer, "sk       RefCnt Type  Rmem   User   Inode\n");

	read_lock(&csme_sklist_lock);

	sk_for_each(sk, node, &csme_sklist) {
//		po = csme_sk(sk);

		len += sprintf(buffer + len, "%p %-6d %-4d   %-6u %-6u %-6lu",
				sk,
				atomic_read(&sk->sk_refcnt),
				sk->sk_type,
				atomic_read(&sk->sk_rmem_alloc), sock_i_uid(sk), sock_i_ino(sk));

		buffer[len++] = '\n';

		pos = begin + len;
		if (pos < offset) {
			len = 0;
			begin = pos;
		}
		if (pos > offset + length)
			goto done;
	}

	*eof = 1;

      done:
	read_unlock(&csme_sklist_lock);
	*start = buffer + (offset - begin);
	len -= (offset - begin);
	if (len > length)
		len = length;
	if (len < 0)
		len = 0;

	return len;
}

static int csme_version_read_proc(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
{
	int len = 0;

	len += sprintf(buffer, CSME_VERSION "\n");

	return len;
}


int csme_create_proc(void)
{
	if (!proc_mkdir ("net/csmencaps", 0)) {
		printk (KERN_ERR "csme: failed to create net/csmencaps proc entry\n");
		goto err0;
	}

	if (!create_proc_read_entry("net/csmencaps/version", 0, 0, csme_version_read_proc, NULL)) {
		printk (KERN_ERR "csme: failed to create net/csmencaps/version proc entry\n");
		goto err1;
	}	

	if (!create_proc_read_entry("net/csmencaps/sockets", 0, 0, csme_socket_read_proc, NULL)) {
		printk (KERN_ERR "csme: failed to create net/csmencaps/sockets proc entry\n");
		goto err2;
	}

	return 0;

err2:
	remove_proc_entry("net/csmencaps/version", NULL);

err1:
	remove_proc_entry("net/csmencaps", NULL);

err0:
	return -EINVAL;
}

void csme_remove_proc(void)
{

	remove_proc_entry("net/csmencaps/sockets", NULL);

	remove_proc_entry("net/csmencaps/version", NULL);

	remove_proc_entry("net/csmencaps", NULL);
}
#endif

/**
 * channel_add - allocate a new channel and add channel to the target channel table
 *
 * Must be called from user context only
 *
 */
static struct csme_channel *channel_add(struct csme_target *target, unsigned short channelid)
{
	struct csme_channel *channel;

	if (channelid >= CSME_MAX_CHANNELS) {
		printk(KERN_ERR "csme: ch(%d, %#lx) out of range\n", channelid, (unsigned long) target);
		goto err;
	}
	
	channel = (struct csme_channel *)kmalloc(sizeof(struct csme_channel), GFP_KERNEL);
	if (!channel) {
		printk(KERN_ERR "csme: kmalloc() failed ch(%d, %#lx)\n", channelid, (unsigned long) target);
		goto err;
	}
	
	memset(channel, 0, sizeof(struct csme_channel));
	channel->id = channelid;

	memset(channel->tx_seq_number, 0, sizeof(channel->tx_seq_number));
	memset(channel->last_rx_seq_number, 0xff, sizeof(channel->last_rx_seq_number));

	skb_queue_head_init(&channel->tx_queue);

	channel->target = target;
	init_timer(&channel->timer);

	channel->timer.function = handle_channel_timeout;
	channel->timer.data = (unsigned long)(channel);

	spin_lock_init(&channel->lock);

	write_lock_bh(&target->lock);

	target->channel[channel->id] = channel;

	write_unlock_bh(&target->lock);

//	printk(KERN_INFO "csme: ch(%d, %#lx) added\n", channel->id, (unsigned long) target);

	return channel;

err:
	return NULL;
}

/**
 * __channel_remove - remove channel from table and free it
 *
 * This function must be called holding the target->lock for writing
 */
static void channel_remove(struct csme_target *target, struct csme_channel *channel)
{
//	printk(KERN_INFO "csme: ch(%d, %#lx) removed\n", channel->id, (unsigned long) channel->target);

	channel_reset(channel);
	target->channel[channel->id] = NULL;

	kfree(channel);
}

/**
 * channel_find - find a channel inside a target's channel list
 *
 *
 */
static struct csme_channel *channel_find(struct csme_target *target, unsigned short channelid)
{
	struct csme_channel *channel = NULL;

	if (channelid >= CSME_MAX_CHANNELS) {
		printk(KERN_ERR "csme: ch(%d, %#lx) out of range\n", channelid, (unsigned long) target);
		goto out;
	}

	read_lock(&target->lock);

	channel = target->channel[channelid];

	read_unlock(&target->lock);

out:

	return channel;
}

/**
 * channel_reset - reset the resources occupied by a channel
 *
 *
 */
static void __channel_reset(struct csme_channel *channel)
{
	del_timer(&channel->timer);

	skb_queue_purge(&channel->tx_queue);

	memset(channel->tx_seq_number, 0, sizeof(channel->tx_seq_number));
	memset(channel->last_rx_seq_number, 0xff, sizeof(channel->last_rx_seq_number));

//	printk(KERN_INFO "csme: ch(%d, %#lx) reset\n", channel->id, (unsigned long) channel->target);
}

static void channel_reset(struct csme_channel *channel)
{
	spin_lock_bh(&channel->lock);

	__channel_reset(channel);

	spin_unlock_bh(&channel->lock);
}

/**
 * target_add - allocate new target and add it to the target list
 *
 * Must be called from user context only
 *
 */
static struct csme_target *target_add(unsigned char *scsme_devmac, int scsme_ifindex)
{
	struct csme_target *target;

	target = (struct csme_target *)kmalloc(sizeof(struct csme_target), GFP_KERNEL);
	if (!target) {
		printk(KERN_ERR "csme: kmalloc() failed for target\n");
		goto err;
	}

	memset(target, 0, sizeof(struct csme_target));

	rwlock_init(&target->lock);

	target->supvsr_channel.target = target;
	target->supvsr_channel.id = SUPVSR_CH;

	memset(target->supvsr_channel.tx_seq_number, 0, sizeof(target->supvsr_channel.tx_seq_number));
	memset(target->supvsr_channel.last_rx_seq_number, 0xff, sizeof(target->supvsr_channel.last_rx_seq_number));

	skb_queue_head_init(&target->supvsr_channel.tx_queue);

	init_timer(&target->supvsr_channel.timer);
	target->supvsr_channel.timer.function = handle_channel_timeout;
	target->supvsr_channel.timer.data = (unsigned long)(&target->supvsr_channel);

	spin_lock_init(&target->supvsr_channel.lock);

	target->boot_channel.target = target;
	target->boot_channel.id = BOOT_CH;

	memset(target->boot_channel.tx_seq_number, 0, sizeof(target->boot_channel.tx_seq_number));
	memset(target->boot_channel.last_rx_seq_number, 0xff, sizeof(target->boot_channel.last_rx_seq_number));

	skb_queue_head_init(&target->boot_channel.tx_queue);

	init_timer(&target->boot_channel.timer);
	target->boot_channel.timer.function = handle_channel_timeout;
	target->boot_channel.timer.data = (unsigned long)(&target->boot_channel);

	spin_lock_init(&target->boot_channel.lock);

	memcpy(target->macaddr, scsme_devmac, ETH_ALEN);
	target->scsme_ifindex = scsme_ifindex;

	target_create_proc(target);

	write_lock_bh(&csme_lock);

	list_add(&target->list, &csme_target_list);

	write_unlock_bh(&csme_lock);

/*	printk(KERN_INFO "csme: target(%#lx) added\n", (unsigned long) target);*/

	return target;

err:
	return NULL;
}

/**
 * __target_remove - remove target from list and free it
 *
 * You must call this function holding the csme_lock
 */
static void __target_remove(struct csme_target *target)
{
	struct csme_channel *channel;
	int i;
	
//	printk(KERN_INFO "csme: target(%#lx) removed\n", (unsigned long) target);
	write_lock_bh(&target->lock);
	
	for (i = 0; i < CSME_MAX_CHANNELS; i++) {
		channel = target->channel[i];
		if (channel)
			channel_remove(target, channel);
	}

	write_unlock_bh(&target->lock);

	channel_reset(&target->boot_channel);
	channel_reset(&target->supvsr_channel);

	target_remove_proc(target);

	list_del(&target->list);

	kfree(target);
}

/**
 * target_find - find a target in the target list based on dest MAC address
 *
 *
 */
static struct csme_target *target_find(unsigned char *scsme_devmac)
{
	struct csme_target *target;

	read_lock(&csme_lock);

	list_for_each_entry(target, &csme_target_list, list)
		if (macAddrEqual(target->macaddr, scsme_devmac))
			goto match;

	read_unlock(&csme_lock);

	return NULL;

match:
	read_unlock(&csme_lock);

	return target;
}

/**
 * target_reset -
 *
 *
 */
static void target_reset(struct csme_target *target)
{
	int i;
	
	write_lock_bh(&target->lock);

	channel_reset(&target->boot_channel);
	channel_reset(&target->supvsr_channel);

	for (i = 0; i < CSME_MAX_CHANNELS; i++) 
		if (target->channel[i])
			channel_reset(target->channel[i]);

	write_unlock_bh(&target->lock);
}

static void target_change_mac(struct csme_target *target, unsigned char *newmac)
{
	/* make sure that another target with the same mac doesn't already exist*/
	if (target_find(newmac) == NULL)
	{
		/* remove the profile for the current mac*/
		target_remove_proc(target);

		write_lock_bh(&target->lock);
		/* Set the new MAC address for the target  */
		memcpy(target->macaddr, newmac, ETH_ALEN);
		write_unlock_bh(&target->lock);

		/* create a proc file for the new MAC*/
		target_create_proc(target);

/*             printk(KERN_INFO "csme: target(%#lx) changed to: ", (unsigned long) target);
               printMacAddr(target->macaddr);*/
       }
}

/**
 * csme_ntoh - converts message payload
 *
 * must be called with skb->data pointing to first api header
 *
 */
static int csme_ntoh(struct sk_buff *skb)
{
	struct csme_hdr *csme_hdr = (struct csme_hdr *) skb_network_header(skb);
	struct api_hdr *api_hdr;
	u8 *next_marker;
//	u8 tmp;
//	int i;
	int cmds;

	if (csme_hdr->opcode == __constant_htons(CSME_OPCODE_CONTROL)) {

		if (csme_hdr->endianess) {
			next_marker = skb->data;

			cmds = 0;
			while (get_unaligned((u32 *)next_marker) && ((next_marker + API_HDR_LEN) < skb->tail)) {
				api_hdr = (struct api_hdr *) next_marker;

				/* avoid infinite loop if message is corrupted */
				if (api_hdr->length < API_HDR_LEN)
					break;

				/* FIXME transform message payload if needed */

				cmds++;

				next_marker += (api_hdr->length + 3) & ~0x3;
				if (next_marker > skb->tail)
					break;
			}
		
			/* adjust skb size */
			skb->tail = next_marker;
			skb->len = skb->tail - skb->data;
		} else {
#if 0
			api_hdr = (struct api_hdr *) skb->data;

			tmp = api_hdr->cmd_type;
			api_hdr->cmd_type = api_hdr->cmd_class;
			api_hdr->cmd_class = tmp;

			/* FIXME should this be done always by 16bit blocks or not? */
			for (i = 0; i < (skb->len - API_HDR_BOOT_LEN) / 2; i++)
				((u16 *)skb->data)[API_HDR_BOOT_LEN/2 + i] = cpu_to_be16(((u16 *)skb->data)[API_HDR_BOOT_LEN/2 + i]);
#endif
			cmds = 1;
		}
	} else
		cmds = 1;

	return cmds;
}


/**
 * csme_tx_ack -
 *
 *
 */
static int csme_tx_ack(struct csme_target *target, struct sk_buff *skb)
{
	struct ethhdr *eth_hdr = (struct ethhdr *) skb_mac_header(skb);
	struct csme_hdr *csme_hdr = (struct csme_hdr *) skb_network_header(skb);
	struct csme_hdr *ncsme_hdr;
	struct net_device *dev;
	int err;

	atomic_inc(&target->stats.tx_ack);

	dev = dev_get_by_index(&init_net,target->scsme_ifindex);
	if (!dev) {
		printk (KERN_ERR "csme: dev_get_by_index(%d) failed\n", target->scsme_ifindex);
		atomic_inc(&target->stats.tx_ack_err);
		err = -ENXIO;
		goto err0;
	}

	if ((skb = alloc_skb(CSME_HDR_LEN + dev->hard_header_len, GFP_ATOMIC)) == NULL) {
		printk (KERN_ERR "csme: alloc_skb(%d) failed\n", CSME_HDR_LEN + dev->hard_header_len);
		atomic_inc(&target->stats.tx_ack_err);
		err = -ENOMEM;
		goto err1;
	}

//	skb_set_owner_w(skb, sk);

	skb_reserve(skb, dev->hard_header_len);

	/* add csme header */		
	ncsme_hdr = (struct csme_hdr *) skb_put(skb, sizeof(struct csme_hdr));

	skb_reset_network_header(skb);

	ncsme_hdr->channel_nb = csme_hdr->channel_nb;
	ncsme_hdr->seq_number = csme_hdr->seq_number;
	ncsme_hdr->ack_sup = 1;
	ncsme_hdr->cr = 1;
	ncsme_hdr->reserved = 0;
	ncsme_hdr->endianess = csme_hdr->endianess;
	ncsme_hdr->opcode = csme_hdr->opcode;

	/* add ethernet header */
//	if (dev->hard_header)
//		dev->hard_header(skb, dev, ETH_P_CSME, eth_hdr->h_source, NULL, skb->len);
	if (dev->header_ops)
		dev->header_ops->create(skb, dev, ETH_P_CSME, eth_hdr->h_source, NULL, skb->len);

	skb_reset_mac_header(skb);

	skb->protocol = __constant_htons(ETH_P_CSME);
	skb->dev = dev;
	skb->priority = 0; /* FIXME */

	err = -ENETDOWN;
	if (!(dev->flags & IFF_UP)) {
		printk (KERN_ERR "csme: network device is down dev->flags(%#x)\n", dev->flags);
		atomic_inc(&target->stats.tx_ack_err);
		goto err2;
	}

	err = dev_queue_xmit(skb);
	if ((err < 0) || ((err > 0) && (err = net_xmit_errno(err)) != 0)) {
		printk (KERN_ERR "csme: dev_queue_xmit() failed, err(%d)\n", err);
		atomic_inc(&target->stats.tx_ack_err);
	}

	dev_put(dev);

	return err;

err2:
	kfree_skb(skb);

err1:
	dev_put(dev);

err0:
	return err;
}



/**
 * csme_tx_msg_finish -
 *
 *
 */
static int csme_tx_msg_finish(struct csme_target *target, struct sk_buff *skb, struct net_device *dev)
{
	int err;

	if (!dev) {
		dev = dev_get_by_index(&init_net, target->scsme_ifindex);
		if (!dev) {
			printk (KERN_ERR "csme: dev_get_by_index(%d) failed\n", target->scsme_ifindex);
			atomic_inc(&target->stats.tx_msg_err);
			err = -ENXIO;
			goto err0;
		}
	}

	skb_reset_mac_header(skb);

	skb->protocol = __constant_htons(ETH_P_CSME);
	skb->dev = dev;
	skb->priority = skb->sk->sk_priority;

	err = -ENETDOWN;
	if (!(dev->flags & IFF_UP)) {
		printk (KERN_ERR "csme: network device is down dev->flags(%#x)\n", dev->flags);
		atomic_inc(&target->stats.tx_msg_err);
		goto err1;
	}

	/* Now send it */
	err = dev_queue_xmit(skb);
	if ((err < 0) || ((err > 0) && (err = net_xmit_errno(err)) != 0)) {
		printk (KERN_ERR "csme: dev_queue_xmit() failed, err(%d)\n", err);
		atomic_inc(&target->stats.tx_msg_err);
	}

	dev_put(dev);

	return err;

err1:
	dev_put(dev);

err0:
	kfree_skb(skb);

	return err;
}

/**
 * csme_tx_msg_next -
 *
 *
 */
static void csme_tx_msg_next(struct csme_channel *channel)
{
	struct sk_buff *skb, *nskb;
	struct csme_target *target = channel->target;

	/* process the next command in the queue */
	/* start timer if required */

	spin_lock_bh(&channel->lock);

	/* check if queue is empty */
	skb = skb_peek(&channel->tx_queue);
	if (!skb) {
		spin_unlock_bh(&channel->lock);
		goto out;
	}

	if (!((struct csme_hdr *)skb_network_header(skb))->ack_sup) {
		spin_unlock_bh(&channel->lock);

		/* start timer */
		channel->retries = RETRIES;
		channel->timer.expires = jiffies + TIMEOUT;
		add_timer(&channel->timer);

		/* if an error happens, the timer will retransmit the packet */

		nskb = skb_clone(skb, GFP_ATOMIC);
		if (!nskb) {
			printk(KERN_ERR "csme: skb_clone() failed ch(%d, %#lx)\n", channel->id, (unsigned long) target);
			atomic_inc(&target->stats.tx_msg_err);
			goto out;
		}

		skb_set_owner_w(nskb, skb->sk);

		csme_tx_msg_finish(target, nskb, NULL);
	} else {
		skb = skb_dequeue(&channel->tx_queue);

		spin_unlock_bh(&channel->lock);

		/* now transmit the next command/message in the queue */
		if (csme_tx_msg_finish(target, skb, NULL) < 0) {
			printk(KERN_ERR "csme: csme_tx_msg_finish() failed ch(%d, %#lx)\n", channel->id, (unsigned long) target);
			goto out;
		}
	}

out:
	return;
}

/**
 * handle_channel_timeout -
 *
 *
 */
static void handle_channel_timeout(unsigned long data)
{
	struct csme_channel *channel = (struct csme_channel *)data;
	struct csme_target *target = channel->target;
	struct sk_buff *skb, *nskb;

	spin_lock_bh(&channel->lock);

	/* check if queue is empty */
	skb = skb_peek(&channel->tx_queue);
	if (!skb) {
		spin_unlock_bh(&channel->lock);
		printk(KERN_ERR "csme: ch(%d, %#lx) no packet to retransmit\n", channel->id, (unsigned long) target);
		goto out;
	}

	if (channel->retries == 0) {
		/* could not get any ACK. delete it and try to send the next one */

		skb = skb_dequeue(&channel->tx_queue);
		spin_unlock_bh(&channel->lock);

		printk(KERN_ERR "csme: ch(%d, %#lx) final retransmission\n", channel->id, (unsigned long) target);

		atomic_inc(&target->stats.tx_msg_max_retransmissions);

		kfree_skb(skb);

		csme_tx_msg_next(channel);

		goto out;
	}

	spin_unlock_bh(&channel->lock);

	if (channel->retries == RETRIES)
		atomic_inc(&target->stats.tx_msg_retransmitted);

	atomic_inc(&target->stats.tx_msg_total_retransmitted);
	
	channel->retries--;

	/* restart timer */
	channel->timer.expires = jiffies + TIMEOUT;
	add_timer(&channel->timer);

	nskb = skb_clone(skb, GFP_ATOMIC);
	if (!nskb) {
		atomic_inc(&target->stats.tx_msg_err);
		goto out;
	}

	skb_set_owner_w(nskb, skb->sk);

	/* now retransmit the packet */
	csme_tx_msg_finish(target, nskb, NULL);

      out:
	return;
}

/**
 * csme_rx_msg -
 *
 *
 */
static int csme_rx_msg(struct sk_buff *skb, struct csme_target *target, struct csme_channel *channel)
{
	struct csme_hdr *hdr = (struct csme_hdr *) skb_network_header(skb);
	u16 opcode = ntohs(hdr->opcode);

	atomic_inc(&target->stats.rx_msg);
	
	spin_lock_bh(&channel->lock);

	if (hdr->seq_number == channel->last_rx_seq_number[opcode]) {
		/* repeated command */
		spin_unlock_bh(&channel->lock);

		atomic_inc(&target->stats.rx_msg_repeated);

//		printk(KERN_INFO "csme: repeated command(%d) ch(%d, %#lx)\n",  hdr->seq_number, channel->id, (unsigned long) target));

		/* transmit ack if requested but don't forward command to upper layer */
		if (!hdr->ack_sup)
			csme_tx_ack(target, skb);

		goto err;
	}

	/* save the last command sequence number */
	channel->last_rx_seq_number[opcode] = hdr->seq_number;

	spin_unlock_bh(&channel->lock);

	/* it is new message/response */

	if ((hdr->channel_nb == __constant_htons(SUPVSR_CH)) && (opcode == CSME_OPCODE_CONTROL)) {
		struct api_hdr *api_hdr;

		/* Track channel/participant create to reset csme sequence number */

		api_hdr = (struct api_hdr *)skb->data;

		if ((CMD_CLASS_CONFIGURATION_DEVICE == api_hdr->cmd_class)
		    && (CMD_TYPE_CONFIGURATION_RESPONSE == api_hdr->cmd_type)
		    && ((cpu_to_le16(FCODE_SUPV_CREATE_CHANNEL) == api_hdr->func_code) || (cpu_to_le16(FCODE_CONF_CREATE_PARTICIPANT) == api_hdr->func_code))
		    && !((u16 *)skb->data)[4])
		{
			channel = channel_find(target, le16_to_cpu(((u16 *)skb->data)[5]));
			if (channel)
				channel_reset(channel);
		}
	}

	/* transmit ack if requested */
	if (!hdr->ack_sup)
		csme_tx_ack(target, skb);

	return 0;

err:
	return -1;
}


/**
 * csme_rx_ack -
 *
 * ACK received, now handle it
 */
static int csme_rx_ack(struct sk_buff *skb, struct csme_target *target, struct csme_channel *channel)
{
	struct csme_hdr *ack_hdr = (struct csme_hdr *) skb_network_header(skb);
	struct csme_hdr *hdr;

	atomic_inc(&target->stats.rx_ack);

	spin_lock_bh(&channel->lock);

	skb = skb_peek(&channel->tx_queue);
	if (!skb) {
//		printk(KERN_INFO "csme: received ack but no tx packet ch(%d, %#lx)\n", channel->id, (unsigned long) target);
		atomic_inc(&target->stats.rx_ack_wrongseq);
		goto err;
	}

	hdr = (struct csme_hdr *) skb_network_header(skb);

	if (hdr->opcode != ack_hdr->opcode) {
		printk(KERN_INFO "csme: received wrong opcode in ack ch(%d, %#lx)\n", channel->id, (unsigned long) target);
		atomic_inc(&target->stats.rx_ack_err);
		goto err;
	}

	if (hdr->seq_number != ack_hdr->seq_number) {
		printk(KERN_INFO "csme: received out of sequence ack %d %d %#lx ch(%d, %#lx)\n",
			hdr->seq_number, ack_hdr->seq_number,
			(unsigned long) hdr,
			channel->id, (unsigned long) target);
		atomic_inc(&target->stats.rx_ack_wrongseq);
		goto err;
	}

	/* received ACK for the last sent command, so remove it from list */
	skb = skb_dequeue(&channel->tx_queue);

	del_timer_sync(&channel->timer);

	spin_unlock_bh(&channel->lock);

	kfree_skb(skb);

	csme_tx_msg_next(channel);

	return 0;

err:
	spin_unlock_bh(&channel->lock);

	return -1;
}

/**
 * csme_rx -
 *
 *
 */
static int csme_rx(struct sk_buff *skb)
{
	struct ethhdr *eth_hdr;
	struct csme_hdr *csme_hdr;
	struct csme_target *target;
	struct csme_channel *channel;
//	int err;

	eth_hdr = (struct ethhdr *)skb_mac_header(skb);

	target = target_find(eth_hdr->h_source);
	if (!target) {
		printk(KERN_INFO "csme_tx: target_find() failed\n");
		goto drop;
	}

	skb_reset_network_header(skb);
	csme_hdr = (struct csme_hdr *) skb->data;

	skb_pull(skb, CSME_HDR_LEN);

//	dump_csme_hdr(csme_hdr);

	if (!csme_hdr->endianess) {
		if (csme_hdr->channel_nb != __constant_htons(BOOT_CH)) {
			printk(KERN_INFO "csme_rx: boot message in ch(%d, %#lx)\n", ntohs(csme_hdr->channel_nb), (unsigned long) target);

			goto drop;
		}

		channel = &target->boot_channel;		

	} else {
		if (csme_hdr->channel_nb ==  __constant_htons(SUPVSR_CH)) {
			channel = &target->supvsr_channel;
		} else {
			channel = channel_find(target, ntohs(csme_hdr->channel_nb));
			if (!channel) {
				printk(KERN_INFO "csme_rx: channel_find() failed, ch(%d, %#lx)\n", ntohs(csme_hdr->channel_nb), (unsigned long) target);
				goto drop;
			}
		}
	}

	if (csme_hdr->cr) {
		/* it is an ACK packet, handle it and drop it */
		if (csme_rx_ack(skb, target, channel) < 0) {
//			printk(KERN_INFO "csme: csme_rx() failed, ch(%d, %#lx)\n", channel->id, (unsigned long) target);
			goto drop;
		}
		/*
		 * for bootload message there is no separate ACK. CMD_ACK acts as both low level
		 * ACK and response. So it must be handed over to user application. On the other
		 * hand for non-bootloader messages, there is a separate low level ACK which
		 * should not be sent to application layer.
		 */
		if (csme_hdr->endianess) {
			/*
			 * Non bootload message - so don't give it to user
			 */

			goto drop;
		}

	} else {
//		err = 0;
		switch (ntohs(csme_hdr->opcode)) {
		case CSME_OPCODE_CONTROL:
		case CSME_OPCODE_RESERVED:
		case CSME_OPCODE_UNIFIED_DIAGNOSTICS:
		case CSME_OPCODE_REMOTE_MEDIA:
			if (csme_rx_msg(skb, target, channel))
				goto drop;
			
			break;

		case CSME_OPCODE_NOOP:
			goto drop;
			break;

		default:
			/* FIXME give response for an unknown opcode */
			goto drop;
			break;
		}
	}

	return 0;

drop:
	return -1;
}


/**
 * csme_tx - handles csme message transmission
 *
 * Must be called with skb->data pointing to the first api header
 */
static int csme_tx(struct sk_buff *skb, struct sockaddr_csme *saddr, struct net_device *dev)
{
	struct api_hdr_boot *api_hdr_boot;
	struct csme_hdr *csme_hdr;
	struct csme_target *target;
	struct csme_channel *channel;
	struct sk_buff *nskb;
	unsigned char maas_mac[6];
	int err = 0;
	u16 opcode = ntohs(saddr->scsme_opcode);

	switch (opcode) {
	case CSME_OPCODE_CONTROL:
		/* make sure we can read cmd_class and cmd_type */
		if (skb->len < API_HDR_BOOT_LEN) {
			printk (KERN_ERR "csme_tx: skb len(%d) too short\n", skb->len);
			err = -EMSGSIZE;
			goto err_free;
		}

		api_hdr_boot = (struct api_hdr_boot *) skb->data;

//		dump_api_hdr(api_hdr);

		if (!(saddr->scsme_flags & 0x1) && (api_hdr_boot->cmd_class == CMD_CLASS_ETH_BOOT_LDR)
		    && (api_hdr_boot->cmd_type == CMD_TYPE_MAAS_ASSIGN)) {
			if (macAddrEqual(bcastaddr, saddr->scsme_devmac)) {
				/* make sure we can read the MAC address in the payload */
				if (skb->len < API_HDR_BOOT_LEN + 6) {
					printk (KERN_ERR "csme_tx: skb len(%d) too short\n", skb->len);
					err = -EMSGSIZE;
					goto err_free;
				}

				memcpy(maas_mac, skb->data + 6, ETH_ALEN);
				/* OK now change the mac of the target with the mac to be assigned
				to be able to receive the ack */ 
				target = target_find(bcastaddr);
				if (target) {
					target_change_mac(target, maas_mac);
				}
			} else {
				memcpy(maas_mac, skb->data + 6, ETH_ALEN);
				/* OK now change the mac of the target with the mac to be assigned
				to be able to receive the ack */ 
				target = target_find(saddr->scsme_devmac);
				if (target) {
					target_change_mac(target, maas_mac);
				}
			}
 
			target = target_find(maas_mac);
			if (!target) {
				target = target_add(maas_mac, saddr->scsme_ifindex);	/* add into linked list */

				if (!target) {
					printk (KERN_ERR "csme_tx: target_add() failed\n");
					err = -ENOMEM;
					goto err_free;
				}
			} else {
				/* Reset the target */
				target_reset(target);
			}
		} else {
			target = target_find(saddr->scsme_devmac);	/* find dest target matching with saddr->scsme_devmac */

			if (!target) {

				target = target_add(saddr->scsme_devmac, saddr->scsme_ifindex);	/* add into linked list */
				if (!target) {
					printk (KERN_ERR "csme_tx: target_add() failed\n");
					err = -ENOMEM;
					goto err_free;
				}
			}
		}

		atomic_inc(&target->stats.tx_msg);

		if (!(saddr->scsme_flags & 0x1)) {
			channel = &target->boot_channel;
			if (saddr->scsme_channelid != __constant_htons(BOOT_CH)) {
				printk (KERN_ERR "csme_tx: invalid ch(%d, %#lx) for boot message\n", ntohs(saddr->scsme_channelid), (unsigned long) target);
				atomic_inc(&target->stats.tx_msg_err);
				err = -EINVAL;
				goto err_free;
			}
		} else if (saddr->scsme_channelid == __constant_htons(SUPVSR_CH))
			channel = &target->supvsr_channel;
		else {
			channel = channel_find(target, ntohs(saddr->scsme_channelid));
			if (!channel) {
				channel = channel_add(target, ntohs(saddr->scsme_channelid));
				if (!channel) {
					printk (KERN_ERR "csme_tx: channel_add(%d, %#lx) failed\n", ntohs(saddr->scsme_channelid), (unsigned long) target);
					atomic_inc(&target->stats.tx_msg_err);
					err = -ENOMEM;
					goto err_free;
				}
			}
		}

		/* Add message terminator */
		memset(skb->tail, 0, 4);
		skb_put(skb, 4);

		break;

	case CSME_OPCODE_NOOP:
	case CSME_OPCODE_RESERVED:
	case CSME_OPCODE_UNIFIED_DIAGNOSTICS:
	case CSME_OPCODE_REMOTE_MEDIA:
		target = target_find(saddr->scsme_devmac);	/* find dest target matching with saddr->scsme_devmac */

		if (!target) {
			target = target_add(saddr->scsme_devmac, saddr->scsme_ifindex);	/* add into linked list */
			if (!target) {
				printk (KERN_ERR "csme_tx: target_add() failed\n");
				err = -ENOMEM;
				goto err_free;
			}
		}

		atomic_inc(&target->stats.tx_msg);

		if (saddr->scsme_channelid == __constant_htons(SUPVSR_CH))
			channel = &target->supvsr_channel;
		else {
			channel = channel_find(target, ntohs(saddr->scsme_channelid));
			if (!channel) {
				channel = channel_add(target, ntohs(saddr->scsme_channelid));
				if (!channel) {
					printk (KERN_ERR "csme_tx: channel_add(%d, %#lx) failed\n", ntohs(saddr->scsme_channelid), (unsigned long) target);
					atomic_inc(&target->stats.tx_msg_err);
					err = -ENOMEM;
					goto err_free;
				}
			}
		}

		break;

	default:
		err = -EINVAL;
		printk (KERN_ERR "csme_tx: invalid opcode(%x), ch(%d)\n", ntohs(saddr->scsme_opcode), ntohs(saddr->scsme_channelid));
		goto err_free;
		break;
	}

	/* add csme header */		
	csme_hdr = (struct csme_hdr *) skb_push(skb, sizeof(struct csme_hdr));

	skb_reset_network_header(skb);

	csme_hdr->channel_nb = htons(channel->id);
	csme_hdr->ack_sup = target->ack_suppression;
	csme_hdr->cr = 0;
	csme_hdr->reserved = 0;
	csme_hdr->endianess = !(channel == &target->boot_channel);
	csme_hdr->opcode = saddr->scsme_opcode;

//	if (dev->hard_header)
//		dev->hard_header(skb, dev, ETH_P_CSME, saddr->scsme_devmac, NULL, skb->len);
	if (dev->header_ops)
		dev->header_ops->create(skb, dev, ETH_P_CSME, saddr->scsme_devmac, NULL, skb->len);

	spin_lock_bh(&channel->lock);

	csme_hdr->seq_number = channel->tx_seq_number[opcode]++;
	channel->tx_seq_number[opcode] &= 0xF;

	if (skb_queue_empty(&channel->tx_queue)) {
		/* If the queue is empty then we can send */
		if (!csme_hdr->ack_sup) {
			/* If we require an ack then we must: keep a copy for
			future retransmissions and start the retransmission timer. */

			skb_queue_tail(&channel->tx_queue, skb);

			spin_unlock_bh(&channel->lock);

			channel->retries = RETRIES;
			channel->timer.expires = jiffies + TIMEOUT;
			add_timer(&channel->timer);

			nskb = skb_clone(skb, GFP_KERNEL);
			if (nskb)
				skb_set_owner_w(nskb, skb->sk);

		} else {
			spin_unlock_bh(&channel->lock);
			nskb = skb;
		}
	} else {
		/* Can't send so just add it to the queue */
		skb_queue_tail(&channel->tx_queue, skb);
		spin_unlock_bh(&channel->lock);
		goto err_unlock;
	}

	return csme_tx_msg_finish(target, nskb, dev);

err_free:
	kfree_skb(skb);

err_unlock:
	dev_put(dev);

	return err;
}

/**
 * csme_get_socket -
 *
 *
 */
static inline struct sock *csme_get_socket(struct sk_buff *skb)
{
	struct sock *sk;

	/* go through the list of sockets and retrieve the one that matches */
	/* for now a single socket is supported */
	read_lock(&csme_sklist_lock);
	sk = sk_head(&csme_sklist);
	read_unlock(&csme_sklist_lock);

	return sk;
}

/**
 * check_ready_received -
 * This function check if a received broadcast frame could be a READY boot indication
 * sent by a comcerto. If yes it adds a new target structure in the target list. 
 *
 */
static int check_ready_received(struct sk_buff *skb, struct net_device *dev)
{
	struct csme_target *target;
	struct ethhdr *eth_hdr;
	struct api_hdr_boot *api_hdr_boot;

	/* check if the frame is big enough to contain at leat the csme & api boot headers */
	if (skb->len < (sizeof(struct api_hdr_boot) + sizeof(struct ethhdr)))
		goto out;

	/* points to the csme api boot header*/
	api_hdr_boot = (struct api_hdr_boot *) (skb->data + CSME_HDR_LEN);

	/* if it is not a READY Command drop the packet*/
	if ((api_hdr_boot->cmd_type != CMD_TYPE_READY) ||
		(api_hdr_boot->cmd_class != CMD_CLASS_ETH_BOOT_LDR))
		goto out;

	/* if no socket opened do not reply to the READY*/
	if (!(csme_get_socket(skb)))
		goto out;

	/* check if this target exists for this particular device*/
	eth_hdr = (struct ethhdr *)skb_mac_header(skb);

	/* check if this target exists for this particular device*/
	target = target_find(eth_hdr->h_source);

	/* if not check if a target able to handle any READY packet exists*/
	if (!target) {
		target = target_find(bcastaddr);
		/* if we do not have a such target drop the frame*/
		if (!target)
			goto out;

		target_change_mac(target, eth_hdr->h_source);
	}

	printk(KERN_INFO "check_ready_received: Ready frame received from:");
	printMacAddr(target->macaddr);
	return 0;
out:
	return -1;
}


/**
 * csmencaps_rcv -
 *
 *
 */
static int csmencaps_rcv(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, struct net_device *orig_dev)
{
	struct sock *sk;
	int skb_len;

	if (skb->pkt_type == PACKET_BROADCAST) {
		if (check_ready_received(skb, dev))
			goto drop;
	}
	else if (skb->pkt_type != PACKET_HOST)
		goto drop;

	skb->dev = dev;

	if (skb_shared(skb)) {
		struct sk_buff *nskb = skb_clone(skb, GFP_ATOMIC);

		printk(KERN_INFO "csme: cloning skb\n");

		if (nskb == NULL) {
			printk(KERN_ERR "csme: skb_clone() failed\n");

			goto drop;
		}

		kfree_skb(skb);
		skb = nskb;
	}

	if (csme_rx(skb))
		goto drop;

	if (!(sk = csme_get_socket(skb))) {
		printk(KERN_INFO "csme: no open socket, dropping skb\n");

		goto drop;
	}

	if (atomic_read(&sk->sk_rmem_alloc) + skb->truesize >= (unsigned)sk->sk_rcvbuf) {
		printk(KERN_INFO "csme: socket receive buffer full(%d), dropping skb\n", sk->sk_rcvbuf);

		goto drop;
	}

	skb_set_owner_r(skb, sk);

	dst_release((struct dst_entry *)skb->_skb_dst);
	skb->_skb_dst = 0;

	/* FIXME not sure about this */
	/* drop conntrack reference */
	/* nf_reset(skb); */

	/* we need to store the len because after queueing the skb it no longer belongs to us */
	skb_len = skb->len;

	spin_lock(&sk->sk_receive_queue.lock);
	__skb_queue_tail(&sk->sk_receive_queue, skb);
	spin_unlock(&sk->sk_receive_queue.lock);

	sk->sk_data_ready(sk, skb_len);

	return 0;

drop:
	kfree_skb(skb);

	return 0;
}

/**
 * generate_sockaddr -
 *
 *
 */
static void generate_sockaddr(struct sk_buff *skb, struct sockaddr_csme *sa)
{
	struct ethhdr *ethhdr = (struct ethhdr *) skb_mac_header(skb);
	struct csme_hdr *csme_hdr = (struct csme_hdr *) skb_network_header(skb);

	/* form user socket address */
	sa->scsme_family = AF_CSME;
	memcpy(sa->scsme_devmac, ethhdr->h_source, ETH_ALEN);
	sa->scsme_opcode = csme_hdr->opcode;
	sa->scsme_channelid = csme_hdr->channel_nb;
	sa->scsme_flags = csme_hdr->endianess;
	sa->scsme_ifindex = skb->dev->ifindex;
}

/*
 *	Pull a packet from our receive queue and hand it to the user.
 *	If necessary we block.
 */
static int csmencaps_recvmsg(struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len, int flags)
{
	struct sock *sk = sock->sk;
	struct sk_buff *skb;
	struct sockaddr_csme *sa;
	int err;
	int multicmds;

	err = -EINVAL;
	if (flags & ~(MSG_PEEK | MSG_DONTWAIT | MSG_TRUNC)) {
		printk(KERN_ERR "csme: wrong flags(%x)\n", flags);
		goto err0;
	}

	if (len < 0) {
		printk(KERN_ERR "csme: wrong len(%d)\n", len);

		goto err0;
	}

	/*
	 *  Call the generic datagram receiver. This handles all sorts
	 *  of horrible races and re-entrancy so we can forget about it
	 *  in the protocol layers.
	 *
	 *  Now it will return ENETDOWN, if device have just gone down,
	 *  but then it will block.
	 */
	err = 0;
	skb = skb_recv_datagram(sk, flags, flags & MSG_DONTWAIT, &err);

	/*
	 *  An error occurred so return it. Because skb_recv_datagram()
	 *  handles the blocking we don't see and worry about blocking
	 *  retries.
	 */

	if (skb == NULL) {
		printk(KERN_ERR "csme: skb_recv_datagram() failed\n");
		goto err0;
	}

	/*
	 *  You lose any data beyond the buffer you gave. If it worries a
	 *  user program they can ask the device for its MTU anyway.
	 */
	if (err < 0) {
		printk(KERN_ERR "csme: skb_recv_datagram() failed, error(%d)\n", err);

		goto err1;
	}

	msg->msg_namelen = sizeof(struct sockaddr_csme);
	sa = (struct sockaddr_csme *)msg->msg_name;

	/* prepare skbpkt */
	generate_sockaddr(skb, sa);

	multicmds = csme_ntoh(skb);

	err = memcpy_toiovec (msg->msg_iov, skb->data, skb->len);
	if (err) {
		printk(KERN_ERR "csme: memcpy_toiovec() failed, error(%d)\n", err);
		goto err1;
	}

	sock_recv_timestamp(msg, sk, skb);

	/*
	 *  Free or return the buffer as appropriate. Again this
	 *  hides all the races and re-entrancy issues from us.
	 */

	if (msg->msg_control != NULL)
		(((struct csme_cmd *) (msg->msg_control))->noofcommands) = multicmds;

	err = skb->len;	

err1:
	skb_free_datagram(sk, skb);

err0:
	return err;
}

/* send the socket message */
static int csmencaps_sendmsg(struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len)
{
	struct sock *sk = sock->sk;
	struct sockaddr_csme *saddr = (struct sockaddr_csme *)msg->msg_name;
	struct sk_buff *skb;
	struct net_device *dev;
	int err = 0;

	/* Get and verify the address */
	if (saddr == NULL) {
		printk (KERN_ERR "csme: null msg_name\n");
		err = -EINVAL;
		goto out;
	}

	if (msg->msg_namelen < sizeof(struct sockaddr_csme)) {
		printk (KERN_ERR "csme: msg_namelen(%d) too small\n", msg->msg_namelen);
		err = -EINVAL;
		goto out;
	}

	dev = dev_get_by_index(&init_net,saddr->scsme_ifindex);
	if (!dev) {
		printk (KERN_ERR "csme: dev_get_by_index(%d) failed\n", saddr->scsme_ifindex);
		err = -ENXIO;
		goto out;
	}

	if (len < 0) {
		err = -EMSGSIZE;
		printk (KERN_ERR "csme: negative len(%d)\n", len);
		goto out_unlock;
	}
	
	if ((len + CSME_HDR_LEN + dev->hard_header_len) > dev->mtu) {
		err = -EMSGSIZE;
		printk (KERN_ERR "csme: len(%d) too big\n", len + CSME_HDR_LEN + dev->hard_header_len);
		goto out_unlock;
	}

	skb = sock_alloc_send_skb(sk, len + 4 + CSME_HDR_LEN + dev->hard_header_len, msg->msg_flags & MSG_DONTWAIT, &err);
	if (!skb) {
		printk (KERN_ERR "csme: sock_alloc_send_skb() failed\n");
		err = -ENOMEM;
		goto out_unlock;
	}

	skb_reserve(skb, CSME_HDR_LEN + dev->hard_header_len);

	/* copy message from userspace to skb */
	err = memcpy_fromiovec(skb_put(skb, len), msg->msg_iov, len);
	if (err < 0) {
		printk (KERN_ERR "csme: memcpy_fromiovec() failed, error(%d)\n", err);
		goto out_free;
	}

	err = csme_tx(skb, saddr, dev);
	if (err) {
		printk (KERN_ERR "csme: csme_tx() failed\n");
		goto out;
	}

	return len;

out_free:
	kfree_skb(skb);

out_unlock:
	if (dev)
		dev_put(dev);
out:
	return err;
}

/*
 *	Close a CSM_ENCAPS socket. This is fairly simple. We immediately go
 *	to 'closed' state and remove our protocol entry in the device list.
 */
static int csmencaps_release(struct socket *sock)
{
	struct sock *sk = sock->sk;

	if (!sk)
		return 0;

	write_lock_bh(&csme_sklist_lock);
	sk_del_node_init(sk);
	write_unlock_bh(&csme_sklist_lock);

	/*
	 *  Now the socket is dead. No more input will appear.
	 */

	sock_orphan(sk);
	sock->sk = NULL;

	/* Purge queues */

	skb_queue_purge(&sk->sk_receive_queue);

	sock_put(sk);

	return 0;
}

#define BUG_TRAP(x) do { \
	if (unlikely(!(x))) { \
		printk(KERN_ERR "KERNEL: assertion (%s) failed at %s (%d)\n", \
			#x,  __FILE__ , __LINE__); \
	} \
} while(0)
void csmencaps_sock_destruct(struct sock *sk)
{
//	printk(KERN_INFO "csmencaps_sock_destruct(%#lx)\n", (unsigned long) sk);

	BUG_TRAP(atomic_read(&sk->sk_rmem_alloc) == 0);
	BUG_TRAP(atomic_read(&sk->sk_wmem_alloc) == 0);

	if (!sock_flag(sk, SOCK_DEAD)) {
		printk(KERN_INFO "csme: attempt to release alive csme socket: %p\n", sk);
		return;
	}

	atomic_dec(&csme_socks_nr);
#ifdef CSM_ENCAPS_REFCNT_DEBUG
	printk(KERN_DEBUG "csme: socket %p is free, %d are alive\n", sk, atomic_read(&csme_socks_nr));
#endif

}


/* set socket options */
static int csmencaps_setsockopt(struct socket *sock, int level, int optname, char *optval,unsigned int optlen)
{
	int opt;
	struct csme_ackopt ackopt;
	struct csme_target *target;
	struct csme_resetopt resetopt;

	if (level != SOL_CSME)
		return -ENOPROTOOPT;

	if (get_user(opt, (int *)optval))
		return -EFAULT;

	switch (optname) {
	case CSME_ACK_ENABLE:
		if (copy_from_user(&ackopt, optval, sizeof(struct csme_ackopt))) {
			return -EFAULT;
		}
		if (ackopt.ack_suppression > 1)
			return -EINVAL;

		target = target_find(ackopt.scsme_devmac);	/* find target matching with scsme_devmac */
		if (!target) {
			target = target_add(ackopt.scsme_devmac, ackopt.scsme_ifindex);	/* add into linked list */
			if (!target)
				goto out;
		}

		target->ack_suppression = ackopt.ack_suppression;

		break;

	case CSME_TARGET_RESET:
		if (copy_from_user(&resetopt, optval, sizeof(struct csme_resetopt))) {
			return -EFAULT;
		}
		target = target_find(resetopt.scsme_devmac);

		/* if no target there, nothing to reset
		else reassign the original address (broadcast or dev hardmac)*/
		if (target) {
			target_change_mac(target, resetopt.scsme_newmac);
			target_reset(target);
		}

		break;

	default:
		return -ENOPROTOOPT;
	}

out:
	return 0;
}

/* get socket options */
int csmencaps_getsockopt(struct socket *sock, int level, int optname, char *optval, int *optlen)
{
	int len;
	struct csme_ackopt ackopt;
	struct csme_target *target;

	if (level != SOL_CSME)
		return -ENOPROTOOPT;

	if (get_user(len, optlen))
		return -EFAULT;

	switch (optname) {
	case CSME_ACK_ENABLE:
		if (copy_from_user(&ackopt, optval, sizeof(struct csme_ackopt))) {
			return -EFAULT;
		}
		target = target_find(ackopt.scsme_devmac);	/* find target matching with saddr->scsme_devmac */
		if (!target)
			return -EINVAL;

		ackopt.ack_suppression = target->ack_suppression;
		ackopt.scsme_ifindex = target->scsme_ifindex;
		len = sizeof(struct csme_ackopt);
		break;

	default:
		return -ENOPROTOOPT;
	}

	if (len < 0)
		return -EINVAL;

	if (put_user(len, optlen))
		return -EFAULT;

	return copy_to_user(optval, &ackopt, len) ? -EFAULT : 0;
}

/* ioctl's supported */
static int csmencaps_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	struct sock *sk = sock->sk;

	switch (cmd) {
	case SIOCOUTQ:
		{
			int amount = atomic_read(&sk->sk_wmem_alloc);
			return put_user(amount, (int *)arg);
		}
	case SIOCINQ:
		{
			struct sk_buff *skb;
			int amount = 0;

			spin_lock_bh(&sk->sk_receive_queue.lock);
			skb = skb_peek(&sk->sk_receive_queue);
			if (skb)
				amount = skb->len;
			spin_unlock_bh(&sk->sk_receive_queue.lock);
			return put_user(amount, (int *)arg);
		}
	case SIOCGSTAMP:
                        return sock_get_timestamp(sk, (struct timeval *)arg);

	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}

/* protocol operations */
struct proto_ops csme_ops = {
	.family = PF_CSME,

	.owner = THIS_MODULE,
	.release = csmencaps_release,
	.bind = sock_no_bind,
	.connect = sock_no_connect,
	.socketpair = sock_no_socketpair,
	.accept = sock_no_accept,
	.getname = sock_no_getname,
	.poll = datagram_poll,
	.ioctl = csmencaps_ioctl,
	.listen = sock_no_listen,
	.shutdown = sock_no_shutdown,
	.setsockopt = csmencaps_setsockopt,
	.getsockopt = csmencaps_getsockopt,
	.sendmsg = csmencaps_sendmsg,
	.recvmsg = csmencaps_recvmsg,
	.mmap = sock_no_mmap,
	.sendpage = sock_no_sendpage,
};

static struct proto csme_proto = {
	.name	  = "CSME",
	.owner	  = THIS_MODULE,
	.obj_size = sizeof(struct csme_sock),
};

/*
 *	Create a CSM_ENCAPS packet of type SOCK_DGRAM.
 */

static int csmencaps_create(struct net *net, struct socket *sock, int protocol)
{
	struct sock *sk;
	int err;

	if (sock->type != SOCK_DGRAM) {
		printk (KERN_ERR "csme: socket type(%x) not supported\n", sock->type);
		return -ESOCKTNOSUPPORT;
	}

	if (protocol && (protocol != __constant_htons(ETH_P_CSME))) {
		printk (KERN_ERR "csme: socket protocol(%x) not supported\n", protocol);
		return -ESOCKTNOSUPPORT;
	}

	read_lock(&csme_sklist_lock);
	sk = sk_head(&csme_sklist);
	read_unlock(&csme_sklist_lock);

	if (sk) {
		printk (KERN_ERR "csme: socket already created\n");
		return -ESOCKTNOSUPPORT;
	}

	sock->state = SS_UNCONNECTED;
	err = -ENOBUFS;

	sk = sk_alloc(&init_net, PF_PACKET, GFP_KERNEL, &csme_proto);
	
	if (!sk)
		goto out;

	sock->ops = &csme_ops;
	sock_init_data(sock, sk);

	sk->sk_family = PF_CSME;

	sk->sk_destruct = csmencaps_sock_destruct;
	atomic_inc(&csme_socks_nr);

	write_lock_bh(&csme_sklist_lock);
	sk_add_node(sk, &csme_sklist);
	write_unlock_bh(&csme_sklist_lock);

	return (0);
out:
	return err;
}

static struct packet_type csme_hook = {
	.func = csmencaps_rcv,
	.type = __constant_htons(ETH_P_CSME),
};

static void __exit csmencaps_exit(void)
{
	struct csme_target *target, *tmp1;

	write_lock_bh(&csme_lock);

	list_for_each_entry_safe(target, tmp1, &csme_target_list, list) {

		__target_remove(target);

	}

	write_unlock_bh(&csme_lock);

#ifdef CONFIG_PROC_FS
	csme_remove_proc();
#endif

	dev_remove_pack(&csme_hook);

	sock_unregister(PF_CSME);
}

static struct net_proto_family csmencaps_family_ops = {
	.family = PF_CSME,
	.create = csmencaps_create,
	.owner = THIS_MODULE,
};

static int __init csmencaps_init(void)
{
	int rc;

	rwlock_init(&csme_lock);

	if ((rc = sock_register(&csmencaps_family_ops)) < 0) {
		printk (KERN_ERR "csme: sock_register() failed\n");
		goto err0;
	}

	dev_add_pack(&csme_hook);

#ifdef CONFIG_PROC_FS
	if ((rc = csme_create_proc()) < 0 ) {
		printk (KERN_ERR "csme: csme_create_proc() failed\n");
		goto err1;
	}

#endif
	return 0;

#ifdef CONFIG_PROC_FS
err1:
	dev_remove_pack(&csme_hook);

	sock_unregister(PF_CSME);
#endif
	
err0:
	return rc;
}

module_init(csmencaps_init);
module_exit(csmencaps_exit);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
