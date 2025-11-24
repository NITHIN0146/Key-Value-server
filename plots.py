import pandas as pd
import matplotlib.pyplot as plt
import sys

if len(sys.argv) < 2:
    print("Error: No CSV file provided.")
    print("Usage: python3 csv_plot.py <csv_file>")
    sys.exit(1)

csv_file = sys.argv[1]

try:
    df = pd.read_csv(csv_file)
    print(f"Successfully loaded: {csv_file}")
    
    df.columns = df.columns.str.strip()
    
    print("Columns found:", df.columns.tolist())

    fig, ax1 = plt.subplots(figsize=(10, 6))

    color = 'tab:blue'
    ax1.set_xlabel('VUs (Virtual Users)')
    ax1.set_ylabel('Throughput (TPS)', color=color)
    ax1.plot(df["vus"], df["tps"], marker='o', color=color, label='TPS')
    ax1.tick_params(axis='y', labelcolor=color)
    ax1.grid(True, linestyle='--', alpha=0.6)

    ax2 = ax1.twinx()
    color = 'tab:red'
    ax2.set_ylabel('Avg Latency (ms)', color=color)
    ax2.plot(df["vus"], df["avg"], marker='x', linestyle='--', color=color, label='Avg Latency')
    ax2.tick_params(axis='y', labelcolor=color)

    plt.title(f'Load Test Results: {csv_file}')
    fig.tight_layout()

    output_img = csv_file.replace('.csv', '.png')
    plt.savefig(output_img)
    print(f"Plot saved to: {output_img}")
    

except KeyError as e:
    print(f"Column name error: {e}")
    print(f"Available columns are: {list(df.columns)}")
    print("Please update the script to match these column names.")
except Exception as e:
    print(f"An error occurred: {e}")