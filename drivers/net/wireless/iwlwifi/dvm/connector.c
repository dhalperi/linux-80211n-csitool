/******************************************************************************
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2008-2011 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110,
 * USA
 *
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.GPL.
 *
 * Contact Information:
 * Daniel Halperin <dhalperi@cs.washington.edu>
 * University of Washington CSE, 185 Stevens Way AC101, Seattle, WA 98195-2350
 * Anmol Sheth <anmol.n.sheth@intel.com>
 * Intel Labs Seattle, 1100 NE 45th St, Seattle, WA 98105
 *
 *****************************************************************************/

#include "connector.h"
#include <linux/export.h>

/**
 * This struct is required for the connector interface to be set up.
 */
struct cb_id connector_id = {
	.idx = CN_IDX_IWLAGN,
	.val = CN_VAL_IWLAGN,
};
EXPORT_SYMBOL(connector_id);
char *connector_name = "iwlagn";
EXPORT_SYMBOL(connector_name);
static struct work_struct connector_work_s;

static struct iwl_priv *priv;
static spinlock_t connector_lock;

#define CN_BUF_SIZE	1024
static struct cn_msg *cn_msg_buf[CN_BUF_SIZE];
static u32 cn_buf_read, cn_buf_write;

/**
 * Enqueues a connector message into the buffer. Will drop and free if full.
 */
static void connector_enqueue_msg(struct cn_msg *m)
{
	unsigned long flags;
	u32 start, end;

	/* Get read and write indices */
	spin_lock_irqsave(&connector_lock, flags);
	start = cn_buf_read;
	end = cn_buf_write;
	spin_unlock_irqrestore(&connector_lock, flags);

	/* Make sure space for message */
	if ((end + 1) % CN_BUF_SIZE == start) {
		printk(KERN_ERR "%s: buffer full; connector message dropped\n",
				__func__);
		kfree(m);
		return;
	}

	/* Make sure priv is known */
	if (priv == NULL) {
		printk(KERN_ERR "%s: priv is NULL\n", __func__);
		kfree(m);
		return;
	}

	/* Enqueue message */
	cn_msg_buf[end] = m;
	spin_lock_irqsave(&connector_lock, flags);
	cn_buf_write = (end + 1) % CN_BUF_SIZE;
	spin_unlock_irqrestore(&connector_lock, flags);

	/* Schedule tasklet */
	queue_work(priv->workqueue, &connector_work_s);
}

/**
 * Sends and frees all the queued messages.
 */
static void connector_send_all(void)
{
	u32 i;
	unsigned long flags;
	u32 start, end;

	/* Get current index */
	spin_lock_irqsave(&connector_lock, flags);
	start = cn_buf_read;
	end = cn_buf_write;
	spin_unlock_irqrestore(&connector_lock, flags);

	/* Loop through buffer sending messages */
	for (i = start; i != end; i = ((i+1) % CN_BUF_SIZE)) {
		cn_netlink_send(cn_msg_buf[i], 0, 0, GFP_ATOMIC);
		kfree(cn_msg_buf[i]);
	}

	/* Set index back to zero, if it has reached 1024 */
	spin_lock_irqsave(&connector_lock, flags);
	cn_buf_read = end;
	spin_unlock_irqrestore(&connector_lock, flags);
}
static void connector_work(struct work_struct *trash)
{
	connector_send_all();
}

/**
 * Sends the message over the kernel connector to a userspace program.
 */
void connector_send_msg(const u8 *data, const u32 size, const u8 code)
{
	struct cn_msg *m;
	u8 *buf;
	u32 payload_size;

	/* Payload + 1-byte "code" */
	payload_size = size + 1 + sizeof(struct cn_msg);
	m = kmalloc(payload_size, GFP_ATOMIC);
	if (m == NULL) {
		printk(KERN_ERR "%s: malloc failed\n", __func__);
		return;
	}
	buf = ((u8 *) m) + sizeof(struct cn_msg);

	/* Set up message */
	memcpy(&m->id, &connector_id, sizeof(struct cb_id));
	m->seq = 0;
	m->len = size + 1;
	buf[0] = code;
	memcpy(&buf[1], data, size);

	/* Enqueue message -- may free on failure */
	connector_enqueue_msg(m);

	return;
}
EXPORT_SYMBOL(connector_send_msg);

void connector_callback(struct cn_msg *msg, struct netlink_skb_parms *nsp)
{
	printk(KERN_ERR "%s: %lu: unhandled message idx=%x, val=%x, seq=%u, ack=%u, len=%d.\n",
		__func__, jiffies, msg->id.idx, msg->id.val,
	msg->seq, msg->ack, msg->len);
}
EXPORT_SYMBOL(connector_callback);
int iwlagn_register_connector(void)
{
	int ret;

	/* Try and setup the connector callback */
	ret = cn_add_callback(&connector_id, connector_name,
			connector_callback);
	if (ret)
		return ret;

	/* Init connector message queue, lock, and tasklet */
	cn_buf_read = 0;
	cn_buf_write = 0;
	spin_lock_init(&connector_lock);
	INIT_WORK(&connector_work_s, connector_work);

	printk(KERN_INFO "iwlagn: connector callback registered\n");
	return 0;
}
EXPORT_SYMBOL(iwlagn_register_connector);
void iwlagn_unregister_connector(void)
{
	u32 i;
	unsigned long flags;

	/* Reset the connector state */
	spin_lock_irqsave(&connector_lock, flags);
	for (i = cn_buf_read; i != cn_buf_write; i = (i+1) % CN_BUF_SIZE)
		kfree(cn_msg_buf[i]);
	spin_unlock_irqrestore(&connector_lock, flags);

	/* Deregister connector */
	cn_del_callback(&connector_id);
	printk(KERN_INFO "iwlagn: connector callback deregistered\n");
}
EXPORT_SYMBOL(iwlagn_unregister_connector);
void iwl_connector_set_priv(struct iwl_priv *p)
{
	unsigned long flags;

	spin_lock_irqsave(&connector_lock, flags);
	priv = p;
	spin_unlock_irqrestore(&connector_lock, flags);
}
EXPORT_SYMBOL(iwl_connector_set_priv);
