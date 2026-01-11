# UK-OUI-SPY Visualization Tool

Web-based visualization and analysis tool for UK-OUI-SPY detection logs.

## Features

✨ **Interactive Dashboard**
- Real-time statistics
- Category breakdowns
- Relevance analysis
- Top manufacturers

📊 **Visual Charts**
- Category distribution bar charts
- Relevance level visualization
- Top 10 manufacturers
- Responsive design

🔍 **Advanced Filtering**
- Filter by relevance (HIGH/MEDIUM/LOW)
- Filter by category
- Filter by manufacturer
- Real-time updates

📱 **Responsive Design**
- Works on desktop, tablet, and mobile
- Touch-friendly interface
- Optimized for all screen sizes

## Quick Start

### 1. Export Log from Device

1. Power off your UK-OUI-SPY device
2. Remove the microSD card
3. Insert into your computer
4. Locate `detections.csv` file

### 2. Open Visualization Tool

**Option A - Local File (Recommended)**
1. Open `detections_viewer.html` in any modern web browser
2. Click "Load detections.csv"
3. Select your CSV file
4. Data loads instantly - no upload needed!

**Option B - Drag & Drop (if supported)**
1. Open the HTML file
2. Drag your CSV file onto the upload area

### 3. Explore Your Data

- **Statistics Grid**: Overview of total detections, devices, and risk levels
- **Charts**: Visual breakdown by category, relevance, and manufacturer
- **Detection List**: Scrollable list with all details
- **Filters**: Narrow down results by criteria

## Features Walkthrough

### Statistics Dashboard

```
┌─────────────────────────────────────────────────────┐
│  Total Detections  │  Unique Devices  │  High Risk  │
│        127         │        23        │      67     │
└─────────────────────────────────────────────────────┘
```

Shows:
- **Total Detections**: All logged events
- **Unique Devices**: Distinct MAC addresses
- **Manufacturers**: Number of different makers
- **High Risk**: High-relevance device count
- **Avg RSSI**: Mean signal strength

### Category Distribution Chart

Visual breakdown of detected device types:
- CCTV (red bars)
- ANPR (orange bars)
- Drones (purple bars)
- Body Cameras (red bars)
- Cloud CCTV (blue bars)
- Dash Cams (cyan bars)

### Filtering System

**Relevance Filter:**
- All (default)
- HIGH - Police/government equipment
- MEDIUM - Retail/transport systems
- LOW - Consumer devices

**Category Filter:**
- All surveillance types
- CCTV only
- ANPR only
- Drones only
- etc.

**Manufacturer Filter:**
- All makers
- Hikvision
- Axis Communications
- DJI
- etc.

### Detection Cards

Each detection shows:
```
┌───────────────────────────────────────┐
│ 🔴 Hikvision                    [HIGH] │
│ Category: CCTV                         │
│ MAC: A4:DA:32:XX:XX:XX                 │
│ RSSI: -65 dBm                          │
│ Type: BLE                              │
│ Deployment: Police                     │
│ Notes: UK Police/Council CCTV          │
└───────────────────────────────────────┘
```

**Color Coding:**
- 🔴 Red border = HIGH relevance
- 🟡 Yellow border = MEDIUM relevance
- 🟢 Green border = LOW relevance

## Technical Details

### Browser Compatibility

✅ **Supported Browsers:**
- Chrome/Edge 90+
- Firefox 88+
- Safari 14+
- Opera 76+

❌ **Not Supported:**
- Internet Explorer (any version)
- Very old mobile browsers

### File Format

Expected CSV format (from UK-OUI-SPY device):

```csv
Timestamp,MAC,OUI,Manufacturer,Category,Relevance,Deployment,RSSI,Type,Notes
1673456789000,A4:DA:32:45:67:89,A4:DA:32,Hikvision,CCTV,HIGH,Police,-65,BLE,UK Police CCTV
```

**Columns:**
- Timestamp: Unix timestamp (milliseconds)
- MAC: Full MAC address
- OUI: First 3 bytes of MAC
- Manufacturer: Device maker
- Category: Device type (CCTV, ANPR, etc.)
- Relevance: HIGH, MEDIUM, or LOW
- Deployment: Typical use case
- RSSI: Signal strength (dBm)
- Type: BLE or WiFi
- Notes: Additional information

### Privacy & Security

🔒 **100% Local Processing**
- No data uploaded to servers
- All processing done in your browser
- No internet connection required
- Your data stays on your computer

**File Access:**
- Uses browser's File API
- Read-only access
- No file system modifications
- Temporary in-memory processing

## Example Analysis

### Scenario: City Centre Walk

**Results:**
- 127 total detections
- 23 unique devices
- 52% HIGH relevance
- Top manufacturer: Hikvision (41%)
- Most common: CCTV (70%)
- Average RSSI: -68 dBm

**Insights:**
- High concentration of police-grade CCTV
- Several body cameras detected (RSSI > -50)
- Mix of BLE and WiFi devices
- Strong signals indicate close proximity

### Scenario: Residential Area

**Results:**
- 34 total detections
- 18 unique devices
- 15% HIGH relevance
- Top manufacturer: Ring (35%)
- Most common: Doorbell Cam (45%)
- Average RSSI: -85 dBm

**Insights:**
- Primarily consumer devices
- Low-relevance equipment
- Weak signals (far away)
- Typical residential surveillance

## Exporting Results

### Browser Screenshot
1. Filter to your desired view
2. Use browser screenshot tool (F12 → Screenshot)
3. Save for reports/documentation

### Data Export
While the viewer is read-only, you can:
1. Copy detection details
2. Take screenshots of charts
3. Use browser Print → Save as PDF
4. Process original CSV with Python tools

## Advanced Usage

### Comparing Multiple Sessions

1. Open visualization tool
2. Load first CSV
3. Take screenshots of statistics
4. Refresh page
5. Load second CSV
6. Compare results

### Combining Data

To merge multiple CSV files:

```bash
# Linux/Mac
cat detections_*.csv > combined.csv
# Remove duplicate headers
sed -i '2,${/^Timestamp/d;}' combined.csv
```

```powershell
# Windows PowerShell
Get-Content detections_*.csv | Set-Content combined.csv
```

Then load `combined.csv` in viewer.

### Filtering Tips

**Find High-Risk Devices Near You:**
1. Set Relevance filter: HIGH
2. Look for RSSI > -60 (very close)
3. Check Category for body cams/ANPR

**Identify Surveillance Hotspots:**
1. Sort by Manufacturer
2. High count of single manufacturer = surveillance cluster
3. Note deployment type

**Track Specific Device:**
1. Filter by Manufacturer
2. Check MAC address patterns
3. Look for repeated detections

## Troubleshooting

### CSV Won't Load

**Problem**: File not loading
- **Check**: File is valid CSV
- **Check**: Headers match expected format
- **Check**: No corrupted data
- **Fix**: Try opening in text editor to verify

### Charts Not Showing

**Problem**: Statistics show but no charts
- **Check**: Browser JavaScript enabled
- **Check**: Using supported browser
- **Fix**: Try different browser (Chrome recommended)

### Wrong Data Displayed

**Problem**: Numbers seem incorrect
- **Check**: Correct CSV file loaded
- **Check**: No duplicate entries
- **Fix**: Verify data in text editor

### Performance Issues

**Problem**: Slow with large files
- **Limit**: Works best with <10,000 detections
- **Solution**: Split large CSV files
- **Alternative**: Use Python analysis tool for huge datasets

## Tips & Tricks

💡 **Quick Filtering:**
- Start with "All" to see full picture
- Use HIGH filter for security assessment
- Use LOW filter to see consumer devices

💡 **RSSI Understanding:**
- -30 to -50: Very close (0-2m)
- -50 to -70: Near (2-10m)
- -70 to -90: Medium (10-30m)
- Below -90: Far (30m+)

💡 **Pattern Recognition:**
- Multiple Hikvision = likely police/council area
- Ring/Nest cluster = residential neighborhood
- DJI detections = active drone surveillance
- Body cams = police presence

💡 **Export for Presentations:**
- Use browser print-to-PDF
- Capture specific filtered views
- Screenshot individual charts
- Create before/after comparisons

## Future Enhancements

Planned features:
- [ ] Timeline view of detections
- [ ] Geographic mapping (with GPS data)
- [ ] Export to PDF report
- [ ] Signal strength heatmap
- [ ] Device tracking over time
- [ ] Statistical analysis tools
- [ ] Customizable charts
- [ ] Dark mode theme

## Support

For issues with the visualization tool:
1. Check browser console for errors (F12)
2. Verify CSV format is correct
3. Try sample data: `examples/sample_detections.csv`
4. Report issues on GitHub

## License

Part of UK-OUI-SPY project - MIT License
