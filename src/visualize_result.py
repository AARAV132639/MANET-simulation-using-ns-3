import xml.etree.ElementTree as ET
import matplotlib.pyplot as plt

def parse_delay(delay_str):
    """
    Convert ns-3 delay string to seconds.
    Handles ns, ms, s. Ignores invalid values.
    """
    if delay_str is None:
        return 0.0

    delay_str = delay_str.replace("+", "").strip()

    try:
        if delay_str.endswith("ns"):
            return float(delay_str.replace("ns", "")) * 1e-9
        elif delay_str.endswith("ms"):
            return float(delay_str.replace("ms", "")) * 1e-3
        elif delay_str.endswith("s"):
            return float(delay_str.replace("s", ""))
        else:
            return float(delay_str)
    except ValueError:
        # Handles cases like "On", empty strings, metadata
        return 0.0


def extract_metrics(xml_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()

    tx = rx = delay = throughput = 0.0

    # IMPORTANT: only parse FlowStats/Flow
    for flow in root.findall(".//FlowStats/Flow"):
        tx += int(flow.attrib.get("txPackets", 0))
        rx += int(flow.attrib.get("rxPackets", 0))

        delay += parse_delay(flow.attrib.get("delaySum"))
        throughput += float(flow.attrib.get("rxBytes", 0)) * 8.0

    pdr = (rx / tx) * 100 if tx > 0 else 0
    avg_delay = delay / rx if rx > 0 else 0
    throughput_kbps = throughput / 1000

    return pdr, avg_delay, throughput_kbps


scenarios = {
    "Baseline": "baseline_swarm.xml",
    "Blackhole": "blackhole_swarm.xml",
    "Grayhole": "grayhole_swarm.xml"
}

pdrs, delays, throughputs = [], [], []

for name, file in scenarios.items():
    pdr, delay, thr = extract_metrics(file)
    pdrs.append(pdr)
    delays.append(delay)
    throughputs.append(thr)

# ---- PDR ----
plt.figure()
plt.bar(scenarios.keys(), pdrs)
plt.ylabel("Packet Delivery Ratio (%)")
plt.title("PDR Comparison")
plt.grid(axis="y")

# ---- Delay ----
plt.figure()
plt.bar(scenarios.keys(), delays)
plt.ylabel("Average End-to-End Delay (s)")
plt.title("Delay Comparison")
plt.grid(axis="y")

# ---- Throughput ----
plt.figure()
plt.bar(scenarios.keys(), throughputs)
plt.ylabel("Throughput (kbps)")
plt.title("Throughput Comparison")
plt.grid(axis="y")

plt.show()   # ONE show at the end