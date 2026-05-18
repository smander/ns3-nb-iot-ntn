# ns3-nb-iot-ntn: NB-IoT NTN Extensions for ns-3.38

This is a fork of [ns-3.38](https://www.nsnam.org/) extending the LENA-NB
NB-IoT module of Jörke *et al.* (WNS3 2022) with non-terrestrial-network
support: NTN propagation, LEO satellite mobility, and attack-traffic
applications for intrusion-detection research.

Companion repository for the paper

> O. S. Mostovyi. *A Quantized Deep Learning Approach to Intrusion
> Detection in Narrowband IoT Non-Terrestrial Networks*. ICTERI 2026
> (under review).

The classifier, training pipeline, and dataset live in a separate
repository:

> https://github.com/smander/NB-IOT-NTN-IDS

## Added Components

See [`NTN_ADDITIONS.md`](NTN_ADDITIONS.md) for the full list of files
added or modified relative to upstream ns-3.38. The five fork
contributions are:

1. **NTN free-space loss model** (`NtnFreeSpaceLossModel`) — path loss
   plus atmospheric attenuation conformant to 3GPP TR 38.811.
2. **NTN propagation delay model** (`NtnPropagationDelayModel`) — per-packet
   delay from instantaneous slant range.
3. **NTN Doppler model** (`NtnDopplerModel`) — carrier-frequency shift from
   satellite radial velocity.
4. **LEO satellite mobility** (`LeoSatelliteMobilityModel`) — circular-orbit
   position and velocity from SGP4-propagated Two-Line-Element sets.
5. **Attack-traffic applications** — `FloodAttackApp`, `SpoofingApp`,
   `ExfiltrationApp`, and the IDS feature tracer
   (`IdsFeatureTracer`) that joins packet captures with instantaneous
   modem measurements.

A complete example scenario is provided at
`src/lte/examples/nb-iot-ntn-ids-scenario.cc`.

## Build

This follows the standard ns-3.38 build flow:

```bash
./waf configure --enable-examples --enable-tests
./waf build
```

## Run the Example IDS Scenario

```bash
./waf --run nb-iot-ntn-ids-scenario
```

This produces the 918-record CSV that the companion repository consumes.

## Status of NTN Standards Compliance

See [`NTN_STATUS.md`](NTN_STATUS.md) for a feature-by-feature mapping
of the IDS feature schema against 3GPP TR 36.763, TS 36.331, and
TR 38.811, and for the list of NTN features that are
hard-coded in the current release (`ta_common`, `k_offset`) pending
the SIB31-NB broadcast implementation.

## License

ns-3 and all upstream code remain under GPLv2 (see `LICENSE`). The NTN
additions in this fork are released under the same GPLv2 license.

When citing this fork, please cite the accompanying paper (see above).

## Acknowledgement of Upstream

This fork is derived from ns-3.38 by the nsnam consortium and from the
LENA-NB NB-IoT extension by P. Jörke, T. Gebauer, and C. Wietfeld
(*"From LENA to LENA-NB"*, WNS3 2022,
[doi:10.1145/3532577.3532600](https://doi.org/10.1145/3532577.3532600)).
The upstream `README.md`, `AUTHORS`, `CONTRIBUTING.md`, `RELEASE_NOTES`,
and `CHANGES.html` files are preserved unchanged in this repository.
