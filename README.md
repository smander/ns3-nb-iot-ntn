ns3-nb-iot-ntn: NB-IoT NTN Extensions for ns-3.38
==================================================

This is a fork of **ns-3.38** + **LENA-NB** that adds Non-Terrestrial
Network (NTN) support: NTN propagation, LEO satellite mobility, and
attack-traffic applications for intrusion-detection research.

It is the simulation back-end used to generate the released NB-IoT NTN
IDS dataset and is the companion repository for the paper

> O. S. Mostovyi. *A Quantized Deep Learning Approach to Intrusion
> Detection in Narrowband IoT Non-Terrestrial Networks*.
> ICTERI 2026 (under review).

The classifier, training pipeline, quantized model checkpoints, and the
released dataset live in a separate repository:

> https://github.com/smander/NB-IOT-NTN-IDS

---

## What This Fork Adds

Five fork contributions on top of upstream ns-3.38 + LENA-NB:

| # | Component | Class | Purpose |
|---|-----------|-------|---------|
| 1 | NTN free-space loss | `NtnFreeSpaceLossModel` | Path loss + atmospheric attenuation per 3GPP TR 38.811 |
| 2 | NTN propagation delay | `NtnPropagationDelayModel` | Per-packet delay from instantaneous slant range |
| 3 | NTN Doppler | `NtnDopplerModel` | Carrier-frequency shift from satellite radial velocity |
| 4 | LEO satellite mobility | `LeoSatelliteMobilityModel` | Circular-orbit position and velocity from SGP4-propagated TLEs |
| 5 | Attack-traffic apps + IDS feature tracer | `FloodAttackApp`, `C2BeaconApp`, `RachFloodApp`, `SpoofingApp`, `ExfiltrationApp`, `IdsFeatureTracer` | Threat classes T1–T5 on top of CoAP telemetry plus the 20-feature record emitter |

A complete example scenario that produces the released IDS dataset is
provided at `src/lte/examples/nb-iot-ntn-ids-scenario.cc`.

For the file-by-file inventory of added and modified sources, see
[`NTN_ADDITIONS.md`](NTN_ADDITIONS.md). For the 3GPP-compliance status of
each feature in the released dataset, see [`NTN_STATUS.md`](NTN_STATUS.md).

---

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

This produces the 918-record CSV that the companion repository
(`NB-IOT-NTN-IDS`) consumes for training, quantization, and evaluation.

---

## Dataset Generation Configuration

The released dataset is produced under the following defaults of the
example scenario:

- A 24-hour satellite visibility schedule propagated through SGP4 from
  publicly available LEO TLE sets, for a fixed ground terminal at
  47.85°N, 35.12°E (Zaporizhzhia, Ukraine).
- A single NB-IoT cell at 180 kHz uplink bandwidth, transparent
  satellite payload at 550 km altitude, and the TR 38.811 NTN-TDL-C
  channel model.
- Benign CoAP POST telemetry plus five adversary processes implementing
  threat classes T1–T5 (Flood, Botnet C2, RACH-storm, Spoof/Replay,
  Exfiltration).

---

## License

ns-3 and all upstream code in this repository remain under **GPLv2**
(see [`LICENSE`](LICENSE)). The NTN additions in this fork are released
under the same GPLv2 license, by inheritance.

When citing this fork, please cite the accompanying paper.

---

## Acknowledgement of Upstream

This fork is derived from:

- **ns-3.38** by the nsnam consortium (https://www.nsnam.org).
- **LENA-NB**, the NB-IoT extension of ns-3 by P. Jörke, T. Gebauer, and
  C. Wietfeld, *"From LENA to LENA-NB: Implementation and Performance
  Evaluation of NB-IoT and Early Data Transmission in ns-3"*, WNS3 2022,
  doi:[10.1145/3532577.3532600](https://doi.org/10.1145/3532577.3532600).

The original ns-3.38 README content is preserved below in full for
reference. All upstream `AUTHORS`, `CONTRIBUTING.md`, `RELEASE_NOTES`,
and `CHANGES.html` files are likewise preserved unchanged.

---
---

# Original ns-3.38 README (preserved verbatim)

The Network Simulator, Version 3
================================

## Table of Contents:

1) [An overview](#an-open-source-project)
2) [Building ns-3](#building-ns-3)
3) [Running ns-3](#running-ns-3)
4) [Getting access to the ns-3 documentation](#getting-access-to-the-ns-3-documentation)
5) [Working with the development version of ns-3](#working-with-the-development-version-of-ns-3)

Note:  Much more substantial information about ns-3 can be found at
https://www.nsnam.org

## An Open Source project

ns-3 is a free open source project aiming to build a discrete-event
network simulator targeted for simulation research and education.
This is a collaborative project; we hope that
the missing pieces of the models we have not yet implemented
will be contributed by the community in an open collaboration
process.

The process of contributing to the ns-3 project varies with
the people involved, the amount of time they can invest
and the type of model they want to work on, but the current
process that the project tries to follow is described here:
https://www.nsnam.org/developers/contributing-code/

This README excerpts some details from a more extensive
tutorial that is maintained at:
https://www.nsnam.org/documentation/latest/

## Building ns-3

The code for the framework and the default models provided
by ns-3 is built as a set of libraries. User simulations
are expected to be written as simple programs that make
use of these ns-3 libraries.

To build the set of default libraries and the example
programs included in this package, you need to use the
tool 'waf'. Detailed information on how to use waf is
included in the file doc/build.txt

However, the real quick and dirty way to get started is to
type the command
```shell
./waf configure --enable-examples
```

followed by

```shell
./waf
```

in the directory which contains this README file. The files
built will be copied in the build/ directory.

The current codebase is expected to build and run on the
set of platforms listed in the [release notes](RELEASE_NOTES)
file.

Other platforms may or may not work: we welcome patches to
improve the portability of the code to these other platforms.

## Running ns-3

On recent Linux systems, once you have built ns-3 (with examples
enabled), it should be easy to run the sample programs with the
following command, such as:

```shell
./waf --run simple-global-routing
```

That program should generate a `simple-global-routing.tr` text
trace file and a set of `simple-global-routing-xx-xx.pcap` binary
pcap trace files, which can be read by `tcpdump -tt -r filename.pcap`
The program source can be found in the examples/routing directory.

## Getting access to the ns-3 documentation

Once you have verified that your build of ns-3 works by running
the simple-point-to-point example as outlined in 3) above, it is
quite likely that you will want to get started on reading
some ns-3 documentation.

All of that documentation should always be available from
the ns-3 website: https://www.nsnam.org/documentation/.

This documentation includes:

  - a tutorial

  - a reference manual

  - models in the ns-3 model library

  - a wiki for user-contributed tips: https://www.nsnam.org/wiki/

  - API documentation generated using doxygen: this is
    a reference manual, most likely not very well suited
    as introductory text:
    https://www.nsnam.org/doxygen/index.html

## Working with the development version of ns-3

If you want to download and use the development version of ns-3, you
need to use the tool `git`. A quick and dirty cheat sheet is included
in the manual, but reading through the git
tutorials found in the Internet is usually a good idea if you are not
familiar with it.

If you have successfully installed git, you can get
a copy of the development version with the following command:
```shell
git clone https://gitlab.com/nsnam/ns-3-dev.git
```

However, we recommend to follow the Gitlab guidelines for starters,
that includes creating a Gitlab account, forking the ns-3-dev project
under the new account's name, and then cloning the forked repository.
You can find more information in the [manual](https://www.nsnam.org/docs/manual/html/working-with-git.html).
