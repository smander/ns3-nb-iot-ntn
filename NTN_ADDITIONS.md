# NTN Additions Relative to Upstream ns-3.38

This file lists every source file added or modified by the
`ns3-nb-iot-ntn` fork relative to upstream ns-3.38 + LENA-NB. Every other
file in the repository is unchanged upstream code.

## New Files

### LTE module — NTN propagation and mobility

| Path | Purpose |
|------|---------|
| `src/lte/model/ntn-free-space-loss-model.{h,cc}` | TR 38.811 path loss + atmospheric attenuation |
| `src/lte/model/ntn-propagation-delay-model.{h,cc}` | Per-packet delay from slant range |
| `src/lte/model/ntn-doppler-model.{h,cc}` | Carrier-frequency shift from radial velocity |
| `src/lte/model/leo-satellite-mobility-model.{h,cc}` | SGP4-driven circular-orbit mobility |
| `src/lte/model/ntn-constellation-manager.{h,cc}` | Multi-satellite scheduling helper |
| `src/lte/helper/ntn-helper.{h,cc}` | Builder helper that wires the four NTN models onto a SpectrumChannel |

### LTE module — Attack-traffic applications and IDS tracer

| Path | Purpose |
|------|---------|
| `src/lte/model/flood-attack-app.{h,cc}` | T1 Flood traffic generator |
| `src/lte/model/c2-beacon-app.{h,cc}` | T2 Botnet C2 beacon generator |
| `src/lte/model/rach-flood-app.{h,cc}` | T3 RACH-storm traffic generator |
| `src/lte/model/spoofing-app.{h,cc}` | T4 Spoof/Replay traffic generator |
| `src/lte/model/exfiltration-app.{h,cc}` | T5 Exfiltration traffic generator |
| `src/lte/model/ids-feature-tracer.{h,cc}` | Hooks ns-3 traces and modem callbacks to emit the 20-feature IDS record |

### Example scenario

| Path | Purpose |
|------|---------|
| `src/lte/examples/nb-iot-ntn-ids-scenario.cc` | End-to-end driver that produces the released NB-IoT NTN IDS dataset |

## Modified Files

The following upstream files are modified to register the new classes
with the ns-3 build system and to extend LENA-NB internals where
needed:

- `src/lte/wscript` — declare new sources and examples to waf
- `src/lte/helper/lte-helper.{h,cc}` — install hooks for the IDS tracer
  and attack apps

(Use `git log --diff-filter=M -- src/lte/` after first commit to see
the exact diffs once the fork is in version control.)

## Status of NTN Features Used by the IDS Dataset

`NTN_STATUS.md` is the authoritative document for which of the 20 IDS
features are extracted from real ns-3 traces vs. pending SIB31-NB
implementation. The summary as of this release:

- **18 features** are extracted from ns-3 protocol traces and modem
  callbacks (see the "REAL" table in `NTN_STATUS.md`).
- **2 features** (`ta_common`, `k_offset`) are held at LEO-representative
  constants because SIB31-NB broadcast generation is not yet
  implemented; both fields will become dynamic in a future release.

## License

All additions in this fork are licensed under GPLv2, matching upstream
ns-3.
