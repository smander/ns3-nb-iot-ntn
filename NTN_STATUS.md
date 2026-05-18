# NB-IoT NTN Implementation Status

## IDS Feature Extraction Status (20 features)

### REAL — Extracted from ns-3 protocol stack

| # | Feature | Source | 3GPP Reference |
|---|---------|--------|---------------|
| 1 | `ip_payload_length` | `Ipv4Header::GetPayloadSize()` | RFC 791 / RFC 8200 Sec 3 |
| 2 | `ip_next_header` | `Ipv4Header::GetProtocol()` | RFC 791 / RFC 8200 Sec 3 |
| 3 | `ip_hop_limit` | `Ipv4Header::GetTtl()` | RFC 791 / RFC 8200 Sec 3 |
| 4 | `udp_src_port` | `UdpHeader::GetSourcePort()` | RFC 768 |
| 5 | `udp_dst_port` | `UdpHeader::GetDestinationPort()` | RFC 768 |
| 6 | `udp_length` | UDP header from packet | RFC 768 |
| 7 | `coap_type` | Parsed from UDP payload (4-byte CoAP header) | RFC 7252 Sec 3 |
| 8 | `coap_code` | Parsed from UDP payload | RFC 7252 Sec 3 |
| 9 | `coap_tkl` | Parsed from UDP payload | RFC 7252 Sec 3 |
| 10 | `direction` | Tx/Rx callback context | Device-known |
| 11 | `nrsrp` | `LteUePhy::ReportUeMeasurements` trace | 3GPP TS 36.214, Sec 5.1.26 |
| 12 | `nrsrq` | `LteUePhy::ReportUeMeasurements` trace | 3GPP TS 36.214, Sec 5.1.27 |
| 13 | `ce_level` | Derived from real RSRP thresholds | 3GPP TS 36.321, Sec 5.1.1 |
| 14 | `timing_advance` | Computed from MobilityModel node positions | 3GPP TS 36.321, Sec 6.1.3.5 |
| 15 | `tx_power` | `LteUePowerControl` | 3GPP TS 36.101, Sec 6.2.2F |
| 18 | `window_pkt_count` | `Ipv4L3Protocol::Tx/Rx` packet count per window | - |
| 19 | `window_byte_count` | `Ipv4L3Protocol::Tx/Rx` byte sum per window | - |
| 20 | `window_conn_attempts` | `LteUeMac` RACH preamble trace | - |

### PENDING — Requires SIB31-NB Implementation (Sub-project B)

| # | Feature | Current Value | What SIB31-NB Will Provide | 3GPP Reference |
|---|---------|--------------|---------------------------|---------------|
| 16 | `ta_common` | Hardcoded: 130000 (≈2ms LEO in Ts units) | Real value broadcast by eNB in SIB31-NB, computed from satellite ephemeris. Changes dynamically as satellite moves. | 3GPP TS 36.331, Sec 6.7.4 (`SystemInformationBlockType31-NB-r17`) |
| 17 | `k_offset` | Hardcoded: 20 | Real scheduling offset from NTN-adapted NB-IoT scheduler, compensating for satellite propagation delay. Varies per scheduling interval. | 3GPP TS 36.331, Sec 6.7.4; TR 36.763, Sec 6.3.1 |

**When SIB31-NB is implemented (Sub-project B), these features will be extracted from actual RRC broadcast messages, making all 20 IDS features real.**

---

## NTN Protocol Features — Implementation Status

### NOT YET IMPLEMENTED (Sub-project B scope)

| Feature | 3GPP Reference | Impact on Dataset |
|---------|---------------|-------------------|
| SIB31-NB / SIB32-NB broadcast | TS 36.331, Sec 6.7.4 | Enables real `ta_common`, `k_offset` features |
| TA pre-compensation from GNSS + ephemeris | TS 36.331, Sec 6.7.4 | `timing_advance` becomes dynamically varying with satellite position |
| Extended HARQ timers (t-PollRetransmit = 6s) | TS 36.322 (Rel-17 NTN) | Affects packet timing patterns in dataset |
| k-Offset scheduling compensation | TS 36.331, Sec 6.7.4; TR 36.763 | `k_offset` varies per scheduling decision |
| Extended RAR window for satellite delay | TR 36.763 | Affects RACH timing, `window_conn_attempts` pattern |
| timeAlignmentTimer = infinity | TS 36.331 (Rel-17 NTN) | UE stays synchronized through long gaps |
| Doppler pre-compensation at UE uplink | TR 36.763 | No direct feature impact, but affects link quality → RSRP/RSRQ |
| Satellite handover / beam switching | TS 36.331 (SIB32-NB) | Traffic interruption patterns in dataset |
| Pass-aware traffic gating | Application-level | `window_pkt_count` shows bursty pass/gap pattern |

### IMPLEMENTED (Sub-project A)

| Feature | Description |
|---------|-------------|
| NTN free-space path loss model | `NtnFreeSpaceLossModel` — FSPL + atmospheric attenuation |
| NTN propagation delay model | `NtnPropagationDelayModel` — distance-based delay |
| NTN Doppler model | `NtnDopplerModel` — frequency shift from satellite velocity |
| LEO satellite mobility | `LeoSatelliteMobilityModel` — circular orbit position/velocity |
| Real IDS feature extraction | 18/20 features from ns-3 protocol stack |
| CoAP header in packet payload | 4-byte RFC 7252 header prepended by all app classes |
| 6 traffic classes | Benign + 5 attack applications with distinct patterns |

---

## Roadmap

1. **Current (Sub-project A):** 18/20 real features, NTN channel models, attack traffic generation
2. **Next (Sub-project B):** SIB31-NB, TA pre-compensation, extended timers → 20/20 real features
3. **Future:** Full NTN handover, multi-satellite constellation, pass-aware gating
