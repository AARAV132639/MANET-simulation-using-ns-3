# MANET Security Analysis using ns-3

## Overview
This project studies **security attacks in Mobile Ad Hoc Networks (MANETs)** using the **ns-3 network simulator**.  
The goal is to understand how malicious nodes affect routing performance and to compare network behavior under normal and attack scenarios.

---

## Tools Used
- ns-3 Network Simulator
- C++
- Ubuntu (WSL)
- VS Code
- Routing Protocol: AODV
- Wireless: IEEE 802.11b (Ad-hoc)

---

## Project Structure
- `src/`
  - `manet_baseline.cc` – Normal MANET scenario
  - `manet_blackhole.cc` – Fixed-node blackhole attack
- `docs/` – For report and explanations
- `results/` – For output files (CSV/XML later)

The source files are linked to `ns-3-dev/scratch/` using **symbolic links**, so ns-3 itself is not modified.

---

## Experiments Completed

### 1. Baseline MANET
- 30 nodes
- AODV routing
- UDP traffic
- Fixed node positions
- Metrics collected: PDR, delay, throughput

**Result:**  
Baseline network shows stable performance with high packet delivery ratio.

---

### 2. Blackhole Attack
- One malicious node (node 5)
- Malicious node drops all packets
- Topology arranged to force traffic through the attacker

**Result:**  
Packet Delivery Ratio drops to **0%**, proving successful blackhole attack.

---

## Key Observation
If nodes are within wireless range, MANET routing prefers **direct communication** and may bypass intermediate nodes.  
To demonstrate blackhole attacks, the attacker must lie on the forwarding path.

---

## Current Status
- Baseline MANET implemented and verified
- Fixed-node blackhole attack implemented and verified
- Code is stable and ready for extension

---

## To-Do
- Implement grayhole attack
- Add random attacker selection
- Enable mobility scenarios
- Export results and plot graphs
- Compare results with existing research papers

---

## Author
Aarav Kumar