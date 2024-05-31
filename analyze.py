import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import argparse
from datetime import datetime

def load_csv(fname):
    df = pd.read_csv(fname)
    df["time"] =  pd.to_datetime(df['time'], errors='coerce', unit='ms')
    df['acc'] = np.sqrt(df['acc_x']**2 + df['acc_y']**2 + df['acc_z']**2)
    df['mag'] = np.sqrt(df['mag_x']**2 + df['mag_y']**2 + df['mag_z']**2)
    return df

parser = argparse.ArgumentParser()
parser.add_argument("filename")
parser.add_argument("-n", "--nfft", default=256, type=int)
parser.add_argument("-o", "--overlap", default=32, type=int)
parser.add_argument("--end", default=None, type=str)
args = parser.parse_args()

df = load_csv(args.filename)

if args.end:
    end_datetime = pd.to_datetime(args.end)
    time_difference = end_datetime - df["time"].iloc[-1]
    df['time'] += time_difference
    print (f"Start {df['time'].iloc[0]}, end {df['time'].iloc[-1]}")

duration = (df['time'].iloc[-1] - df['time'].iloc[0]).total_seconds()
print (f"Loaded {len(df)} samples from {args.filename} over {duration}s!")

dt = df['time'].diff().dt.total_seconds()[1:]
sample_rate = 1.0 / np.median(dt)

fig, axs = plt.subplots(figsize=(20, 10))
fig.canvas.manager.set_window_title("specgram")
fig.suptitle('Specgram of frequency', fontsize=16)
Pxx, freqs, bins, im = axs.specgram(
        df['acc'], Fs=sample_rate, NFFT=args.nfft, noverlap=args.overlap, cmap="plasma", detrend="mean")
axs.set_xlabel("time (s)")
axs.set_ylabel("freq (hz)")
fig.colorbar(im, ax=axs, label='Intensity (dB)')

num_bins = len(bins)
step = max(1, int(90 * 60 / sample_rate))
axs.set_xticks(bins[::step])
times = df['time'].iloc[0] + pd.to_timedelta(bins[::step], unit="s")
axs.set_xticklabels([f"{time.strftime('%m-%d %H:%M:%S')}" for time in times], rotation=45, ha='right')

plt.show()
