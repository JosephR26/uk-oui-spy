# UK-OUI-SPY Analysis Tools

Python scripts for analyzing detection logs from the UK-OUI-SPY device.

## Requirements

```bash
pip install pandas
```

## Usage

### Basic Analysis

```bash
python analyze_detections.py /path/to/detections.csv
```

This will generate:
- Detection summary statistics
- Category breakdown
- Relevance analysis
- Top manufacturers
- RSSI (proximity) statistics
- High-risk device list
- CSV export of summary data

### Example Output

```
============================================================
DETECTION SUMMARY
============================================================

Total Detections: 127
Unique Devices (MAC): 23
Unique Manufacturers: 8
Time Range: 45.3 minutes

============================================================
CATEGORY BREAKDOWN
============================================================
  CCTV           :   89 ( 70.1%)
  ANPR           :   18 ( 14.2%)
  Drone          :   12 (  9.4%)
  Body Cam       :    8 (  6.3%)

============================================================
HIGH-RISK DEVICE DETECTIONS
============================================================

  Found 67 high-risk detections:

  🔴 Hikvision            | CCTV       | RSSI: -62 | UK Police/Council CCTV
  🔴 DJI                  | Drone      | RSSI: -78 | Police Drones
  🔴 Motorola Solutions   | ANPR       | RSSI: -55 | ANPR Systems
  ...
```

## Advanced Analysis

### Filter by Date Range

```python
import pandas as pd

df = pd.read_csv('detections.csv')
df['DateTime'] = pd.to_datetime(df['Timestamp'], unit='ms')
filtered = df[df['DateTime'] > '2025-01-11 10:00:00']
print(filtered)
```

### Find Strongest Signals

```python
df = pd.read_csv('detections.csv')
strongest = df.nlargest(10, 'RSSI')
print(strongest[['Manufacturer', 'Category', 'RSSI', 'MAC']])
```

### Detection Timeline

```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('detections.csv')
df['DateTime'] = pd.to_datetime(df['Timestamp'], unit='ms')
df.set_index('DateTime').resample('5T').size().plot()
plt.title('Detections per 5 Minutes')
plt.show()
```

## Visualization Ideas

1. **Heatmap** - Plot RSSI by location (with GPS data)
2. **Timeline** - Detections over time
3. **Manufacturer Pie Chart** - Distribution by manufacturer
4. **RSSI Distribution** - Histogram of signal strengths
5. **Category Network Graph** - Relationships between categories and deployments
