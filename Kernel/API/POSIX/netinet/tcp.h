/*
 * Copyright (c) 2021, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/API/POSIX/sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCP_INFO 11 /* Information about this connection. */

#define TCP_NODELAY 10

struct tcp_info {
    uint8_t	tcpi_state;
	uint8_t	tcpi_ca_state;
	uint8_t	tcpi_retransmits;
	uint8_t	tcpi_probes;
	uint8_t	tcpi_backoff;
	uint8_t	tcpi_options;
	uint8_t	tcpi_snd_wscale : 4, tcpi_rcv_wscale : 4;
	uint8_t	tcpi_delivery_rate_app_limited:1, tcpi_fastopen_client_fail:2;

	uint32_t	tcpi_rto;
	uint32_t	tcpi_ato;
	uint32_t	tcpi_snd_mss;
	uint32_t	tcpi_rcv_mss;

	uint32_t	tcpi_unacked;
	uint32_t	tcpi_sacked;
	uint32_t	tcpi_lost;
	uint32_t	tcpi_retrans;
	uint32_t	tcpi_fackets;

	/* Times. */
	uint32_t	tcpi_last_data_sent;
	uint32_t	tcpi_last_ack_sent;     /* Not remembered, sorry. */
	uint32_t	tcpi_last_data_recv;
	uint32_t	tcpi_last_ack_recv;

	/* Metrics. */
	uint32_t	tcpi_pmtu;
	uint32_t	tcpi_rcv_ssthresh;
	uint32_t	tcpi_rtt;
	uint32_t	tcpi_rttvar;
	uint32_t	tcpi_snd_ssthresh;
	uint32_t	tcpi_snd_cwnd;
	uint32_t	tcpi_advmss;
	uint32_t	tcpi_reordering;

	uint32_t	tcpi_rcv_rtt;
	uint32_t	tcpi_rcv_space;

	uint32_t	tcpi_total_retrans;

	uint64_t	tcpi_pacing_rate;
	uint64_t	tcpi_max_pacing_rate;
	uint64_t	tcpi_bytes_acked;    /* RFC4898 tcpEStatsAppHCThruOctetsAcked */
	uint64_t	tcpi_bytes_received; /* RFC4898 tcpEStatsAppHCThruOctetsReceived */
	uint32_t	tcpi_segs_out;	     /* RFC4898 tcpEStatsPerfSegsOut */
	uint32_t	tcpi_segs_in;	     /* RFC4898 tcpEStatsPerfSegsIn */

	uint32_t	tcpi_notsent_bytes;
	uint32_t	tcpi_min_rtt;
	uint32_t	tcpi_data_segs_in;	/* RFC4898 tcpEStatsDataSegsIn */
	uint32_t	tcpi_data_segs_out;	/* RFC4898 tcpEStatsDataSegsOut */

	uint64_t   tcpi_delivery_rate;

	uint64_t	tcpi_busy_time;      /* Time (usec) busy sending data */
	uint64_t	tcpi_rwnd_limited;   /* Time (usec) limited by receive window */
	uint64_t	tcpi_sndbuf_limited; /* Time (usec) limited by send buffer */

	uint32_t	tcpi_delivered;
	uint32_t	tcpi_delivered_ce;

	uint64_t	tcpi_bytes_sent;     /* RFC4898 tcpEStatsPerfHCDataOctetsOut */
	uint64_t	tcpi_bytes_retrans;  /* RFC4898 tcpEStatsPerfOctetsRetrans */
	uint32_t	tcpi_dsack_dups;     /* RFC4898 tcpEStatsStackDSACKDups */
	uint32_t	tcpi_reord_seen;     /* reordering events seen */

	uint32_t	tcpi_rcv_ooopack;    /* Out-of-order packets received */

	uint32_t	tcpi_snd_wnd;	     /* peer's advertised receive window after
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

#ifdef __cplusplus
}
#endif
