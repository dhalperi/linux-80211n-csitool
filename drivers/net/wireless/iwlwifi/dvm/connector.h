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

/**
 * This file defines the connector interface.
 */

#ifndef __iwl_connector_h__
#define __iwl_connector_h__

#include <linux/etherdevice.h>
#include <net/mac80211.h>
#include <asm/unaligned.h>
#include <linux/connector.h>
#include "dev.h"
#include "commands.h"

extern struct cb_id connector_id;
extern char *connector_name;

void connector_send_msg(const u8 *data, const u32 size, const u8 code);
void iwl_connector_set_priv(struct iwl_priv *p);
void connector_tasklet(unsigned long trash);

#define CN_IDX_IWLAGN	(CN_NETLINK_USERS + 0xf)
#define CN_VAL_IWLAGN	0x1

#define IWL_CONN_BFEE_NOTIF	REPLY_BFEE_NOTIFICATION		/* 0xbb */
#define IWL_CONN_RX_PHY		REPLY_RX_PHY_CMD		/* 0xc0 */
#define IWL_CONN_RX_MPDU	REPLY_RX_MPDU_CMD		/* 0xc1 */
#define IWL_CONN_RX		REPLY_RX			/* 0xc3 */
#define IWL_CONN_NOISE		0xd0		/* new ID not a command */
#define IWL_CONN_TX_RESP	REPLY_TX			/* 0x1c */
#define IWL_CONN_TX_BLOCK_AGG	REPLY_COMPRESSED_BA		/* 0xc5 */
#define IWL_CONN_STATUS		0xd1		/* new ID not a command */

enum {
	IWL_CONN_BFEE_NOTIF_MSK		= (1 << 0),
	IWL_CONN_RX_PHY_MSK		= (1 << 1),
	IWL_CONN_RX_MPDU_MSK		= (1 << 2),
	IWL_CONN_RX_MSK			= (1 << 3),
	IWL_CONN_NOISE_MSK		= (1 << 4),
	IWL_CONN_TX_RESP_MSK		= (1 << 5),
	IWL_CONN_TX_BLOCK_AGG_MSK	= (1 << 6),
	IWL_CONN_STATUS_MSK		= (1 << 7),
};

void connector_callback(struct cn_msg *msg, struct netlink_skb_parms *nsp);
int iwlagn_register_connector(void);
void iwlagn_unregister_connector(void);

/*
 * Struct to send TX block aggregation information down to the host.
 */
struct tx_agg_ba_connector_msg {
	u32 successes;
	u32 frame_count;
};

#endif /* __iwl_connector_h__ */
