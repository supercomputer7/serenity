/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>

#define TCP_INFO 11 /* Information about this connection. */

#define TCP_NODELAY 10

struct tcp_info {
    u8	tcpi_state;
	u8	tcpi_ca_state;
	u8	tcpi_retransmits;
	u8	tcpi_probes;
	u8	tcpi_backoff;
	u8	tcpi_options;
	u8	tcpi_snd_wscale : 4, tcpi_rcv_wscale : 4;
	u8	tcpi_delivery_rate_app_limited:1, tcpi_fastopen_client_fail:2;

	u32	tcpi_rto;
	u32	tcpi_ato;
	u32	tcpi_snd_mss;
	u32	tcpi_rcv_mss;

	u32	tcpi_unacked;
	u32	tcpi_sacked;
	u32	tcpi_lost;
	u32	tcpi_retrans;
	u32	tcpi_fackets;

	/* Times. */
	u32	tcpi_last_data_sent;
	u32	tcpi_last_ack_sent;     /* Not remembered, sorry. */
	u32	tcpi_last_data_recv;
	u32	tcpi_last_ack_recv;

	/* Metrics. */
	u32	tcpi_pmtu;
	u32	tcpi_rcv_ssthresh;
	u32	tcpi_rtt;
	u32	tcpi_rttvar;
	u32	tcpi_snd_ssthresh;
	u32	tcpi_snd_cwnd;
	u32	tcpi_advmss;
	u32	tcpi_reordering;

	u32	tcpi_rcv_rtt;
	u32	tcpi_rcv_space;

	u32	tcpi_total_retrans;

	u64	tcpi_pacing_rate;
	u64	tcpi_max_pacing_rate;
	u64	tcpi_bytes_acked;    /* RFC4898 tcpEStatsAppHCThruOctetsAcked */
	u64	tcpi_bytes_received; /* RFC4898 tcpEStatsAppHCThruOctetsReceived */
	u32	tcpi_segs_out;	     /* RFC4898 tcpEStatsPerfSegsOut */
	u32	tcpi_segs_in;	     /* RFC4898 tcpEStatsPerfSegsIn */

	u32	tcpi_notsent_bytes;
	u32	tcpi_min_rtt;
	u32	tcpi_data_segs_in;	/* RFC4898 tcpEStatsDataSegsIn */
	u32	tcpi_data_segs_out;	/* RFC4898 tcpEStatsDataSegsOut */

	u64   tcpi_delivery_rate;

	u64	tcpi_busy_time;      /* Time (usec) busy sending data */
	u64	tcpi_rwnd_limited;   /* Time (usec) limited by receive window */
	u64	tcpi_sndbuf_limited; /* Time (usec) limited by send buffer */

	u32	tcpi_delivered;
	u32	tcpi_delivered_ce;

	u64	tcpi_bytes_sent;     /* RFC4898 tcpEStatsPerfHCDataOctetsOut */
	u64	tcpi_bytes_retrans;  /* RFC4898 tcpEStatsPerfOctetsRetrans */
	u32	tcpi_dsack_dups;     /* RFC4898 tcpEStatsStackDSACKDups */
	u32	tcpi_reord_seen;     /* reordering events seen */

	u32	tcpi_rcv_ooopack;    /* Out-of-order packets received */

	u32	tcpi_snd_wnd;	     /* peer's advertised receive window after
				      * scaling (bytes)
				      */
};

enum {
	TCP_ESTABLISHED = 1,
	TCP_SYN_SENT,
	TCP_SYN_RECV,
	TCP_FIN_WAIT1,
	TCP_FIN_WAIT2,
	TCP_TIME_WAIT,
	TCP_CLOSE,
	TCP_CLOSE_WAIT,
	TCP_LAST_ACK,
	TCP_LISTEN,
	TCP_CLOSING,	/* Now a valid state */
	TCP_NEW_SYN_RECV,

	TCP_MAX_STATES	/* Leave at the end! */
};

#define TCPI_OPT_TIMESTAMPS	1
#define TCPI_OPT_SACK		2
#define TCPI_OPT_WSCALE		4
#define TCPI_OPT_ECN		8 /* ECN was negociated at TCP session init */
#define TCPI_OPT_ECN_SEEN	16 /* we received at least one packet with ECT */
#define TCPI_OPT_SYN_DATA	32 /* SYN-ACK acked data in SYN sent or rcvd */
