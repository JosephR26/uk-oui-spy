#!/usr/bin/env python3
"""
UK-OUI-SPY Log Analysis Tool

Analyzes detection logs from the UK-OUI-SPY device and generates
statistical reports and visualizations.

Usage:
    python analyze_detections.py detections.csv
"""

import pandas as pd
import sys
from datetime import datetime
from collections import Counter

def load_detections(filepath):
    """Load detection CSV file"""
    try:
        df = pd.read_csv(filepath)
        print(f"✓ Loaded {len(df)} detections from {filepath}\n")
        return df
    except FileNotFoundError:
        print(f"✗ Error: File '{filepath}' not found")
        sys.exit(1)
    except Exception as e:
        print(f"✗ Error loading file: {e}")
        sys.exit(1)

def print_summary(df):
    """Print summary statistics"""
    print("=" * 60)
    print("DETECTION SUMMARY")
    print("=" * 60)

    print(f"\nTotal Detections: {len(df)}")
    print(f"Unique Devices (MAC): {df['MAC'].nunique()}")
    print(f"Unique Manufacturers: {df['Manufacturer'].nunique()}")

    # Time range
    if len(df) > 0:
        time_range = (df['Timestamp'].max() - df['Timestamp'].min()) / 1000 / 60
        print(f"Time Range: {time_range:.1f} minutes")

def print_category_breakdown(df):
    """Print breakdown by category"""
    print("\n" + "=" * 60)
    print("CATEGORY BREAKDOWN")
    print("=" * 60)

    category_counts = df['Category'].value_counts()
    for cat, count in category_counts.items():
        pct = (count / len(df)) * 100
        print(f"  {cat:15s}: {count:4d} ({pct:5.1f}%)")

def print_relevance_analysis(df):
    """Print breakdown by relevance level"""
    print("\n" + "=" * 60)
    print("RELEVANCE ANALYSIS")
    print("=" * 60)

    relevance_counts = df['Relevance'].value_counts()
    for rel, count in relevance_counts.items():
        pct = (count / len(df)) * 100
        icon = "🔴" if rel == "HIGH" else "🟡" if rel == "MEDIUM" else "🟢"
        print(f"  {icon} {rel:10s}: {count:4d} ({pct:5.1f}%)")

def print_manufacturer_ranking(df, top_n=10):
    """Print top manufacturers"""
    print("\n" + "=" * 60)
    print(f"TOP {top_n} MANUFACTURERS")
    print("=" * 60)

    mfg_counts = df['Manufacturer'].value_counts().head(top_n)
    for i, (mfg, count) in enumerate(mfg_counts.items(), 1):
        pct = (count / len(df)) * 100
        print(f"  {i:2d}. {mfg:25s}: {count:4d} ({pct:5.1f}%)")

def print_deployment_analysis(df):
    """Print deployment type analysis"""
    print("\n" + "=" * 60)
    print("DEPLOYMENT TYPE ANALYSIS")
    print("=" * 60)

    deploy_counts = df['Deployment'].value_counts()
    for dep, count in deploy_counts.items():
        pct = (count / len(df)) * 100
        print(f"  {dep:15s}: {count:4d} ({pct:5.1f}%)")

def print_rssi_statistics(df):
    """Print RSSI (proximity) statistics"""
    print("\n" + "=" * 60)
    print("SIGNAL STRENGTH (RSSI) STATISTICS")
    print("=" * 60)

    print(f"  Average RSSI: {df['RSSI'].mean():.1f} dBm")
    print(f"  Strongest Signal: {df['RSSI'].max():.1f} dBm")
    print(f"  Weakest Signal: {df['RSSI'].min():.1f} dBm")
    print(f"  Median RSSI: {df['RSSI'].median():.1f} dBm")

    # Proximity breakdown
    print("\n  Proximity Breakdown:")
    very_close = len(df[df['RSSI'] > -50])
    near = len(df[(df['RSSI'] > -70) & (df['RSSI'] <= -50)])
    medium = len(df[(df['RSSI'] > -90) & (df['RSSI'] <= -70)])
    far = len(df[df['RSSI'] <= -90])

    print(f"    Very Close (>-50 dBm):  {very_close:4d} ({very_close/len(df)*100:5.1f}%)")
    print(f"    Near (-50 to -70 dBm):  {near:4d} ({near/len(df)*100:5.1f}%)")
    print(f"    Medium (-70 to -90):    {medium:4d} ({medium/len(df)*100:5.1f}%)")
    print(f"    Far (<-90 dBm):         {far:4d} ({far/len(df)*100:5.1f}%)")

def print_type_breakdown(df):
    """Print BLE vs WiFi breakdown"""
    print("\n" + "=" * 60)
    print("DETECTION TYPE (BLE vs WiFi)")
    print("=" * 60)

    type_counts = df['Type'].value_counts()
    for dtype, count in type_counts.items():
        pct = (count / len(df)) * 100
        print(f"  {dtype:8s}: {count:4d} ({pct:5.1f}%)")

def print_high_risk_devices(df):
    """Print high-risk device detections"""
    print("\n" + "=" * 60)
    print("HIGH-RISK DEVICE DETECTIONS")
    print("=" * 60)

    high_risk = df[df['Relevance'] == 'HIGH']

    if len(high_risk) == 0:
        print("  No high-risk devices detected")
        return

    print(f"\n  Found {len(high_risk)} high-risk detections:\n")

    for _, row in high_risk.head(20).iterrows():
        print(f"  🔴 {row['Manufacturer']:20s} | {row['Category']:10s} | RSSI:{row['RSSI']:4d} | {row['Notes']}")

def export_summary_csv(df, output_file="analysis_summary.csv"):
    """Export summary to CSV"""
    summary = df.groupby(['Manufacturer', 'Category', 'Relevance']).agg({
        'MAC': 'count',
        'RSSI': 'mean'
    }).reset_index()
    summary.columns = ['Manufacturer', 'Category', 'Relevance', 'Count', 'Avg_RSSI']
    summary = summary.sort_values('Count', ascending=False)
    summary.to_csv(output_file, index=False)
    print(f"\n✓ Summary exported to {output_file}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python analyze_detections.py <detections.csv>")
        sys.exit(1)

    filepath = sys.argv[1]

    print("\n" + "=" * 60)
    print("UK-OUI-SPY Detection Log Analyzer")
    print("=" * 60 + "\n")

    # Load data
    df = load_detections(filepath)

    # Run analysis
    print_summary(df)
    print_category_breakdown(df)
    print_relevance_analysis(df)
    print_manufacturer_ranking(df)
    print_deployment_analysis(df)
    print_rssi_statistics(df)
    print_type_breakdown(df)
    print_high_risk_devices(df)

    # Export summary
    export_summary_csv(df)

    print("\n" + "=" * 60)
    print("Analysis complete!")
    print("=" * 60 + "\n")

if __name__ == "__main__":
    main()
