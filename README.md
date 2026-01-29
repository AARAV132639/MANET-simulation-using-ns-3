# MANET Swarm Security Analysis using ns-3

## Project Overview
This project studies the behavior of a **Mobile Ad Hoc Network (MANET)** formed by a small drone swarm under normal conditions and under **routing attacks**. The focus is on understanding how **blackhole and grayhole attacks** affect network performance metrics such as Packet Delivery Ratio (PDR), delay, and throughput.

The project is implemented using the **ns-3 network simulator** with Wi-Fi ad-hoc communication and AODV routing.

This repository represents a **locked, small-scale validated study**. Large-scale swarm simulations and advanced extensions are intentionally planned as a separate follow-up project.

---

## Objectives
- Build a baseline MANET using Wi-Fi and AODV
- Model a **fixed-node blackhole attack**
- Model a **grayhole attack with probabilistic packet dropping**
- Compare baseline vs attack scenarios using quantitative metrics
- Visualize and interpret results correctly

---

## Simulation Setup
- Simulator: ns-3
- Routing protocol: AODV
- MAC/PHY: IEEE 802.11 ad-hoc
- Transport: UDP
- Mobility: Controlled patrol-based movement
- Swarm size: Small unit (5–7 nodes)
- Simulation area: 300 × 300
- Attacks introduced during active patrol

---

## Implemented Scenarios
1. **Baseline MANET**
   - Normal AODV routing
   - No malicious behavior

2. **Blackhole Attack**
   - One fixed malicious node
   - Drops all received packets

3. **Grayhole Attack**
   - One fixed malicious node
   - Drops packets probabilistically
   - Demonstrates stealthy degradation behavior

---

## Metrics Collected
- Packet Delivery Ratio (PDR)
- Average End-to-End Delay
- Aggregate Throughput

Metrics are collected using **FlowMonitor** and exported as XML files.

---

## Visualization
Results are visualized using a Python script that parses FlowMonitor XML output and generates bar plots for:
- PDR comparison
- Delay comparison
- Throughput comparison

This visualization pipeline is stable and reusable for future scaling experiments.

---

## File Structure (Logical)
- `manet_baseline.cc` — baseline MANET simulation
- `manet_blackhole.cc` — fixed-node blackhole attack
- `manet_grayhole.cc` — fixed-node grayhole attack
- `visualize_result.py` — result parsing and plotting
- `README.md` — project documentation

(Source files are symlinked into ns-3 `scratch/` for execution.)

---

## Project Status
**Frozen / Locked**

- Core objectives achieved
- Results validated and visualized
- No further scaling in this repository

Large-scale swarm simulations, energy models, and advanced attack mitigation are planned as a **separate project** to preserve clarity and reproducibility.

---

## Key Insight
Stealthy grayhole attacks may not always reduce packet delivery ratio. Instead, they can degrade **network stability and predictability**, highlighting why PDR alone is insufficient for security evaluation in MANETs.

---

## Future Work (Out of Scope for This Repo)
- Large-scale drone swarms (30+ nodes)
- Energy-aware routing
- Multi-unit coordination
- Attack detection and mitigation
- Mission-level performance analysis

---
