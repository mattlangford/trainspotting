import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import argparse

def load_csv(fname):
    df = pd.read_csv(fname)
    df["time"] =  pd.to_datetime(df['time'], errors='coerce', unit='ms')
    df['acc'] = np.sqrt(df['acc_x']**2 + df['acc_y']**2 + df['acc_z']**2)
    df['mag'] = np.sqrt(df['mag_x']**2 + df['mag_y']**2 + df['mag_z']**2)
    return df

parser = argparse.ArgumentParser()
parser.add_argument("filename")
parser.add_argument("-n", "--nfft", default=256, type=int)
args = parser.parse_args()

df = load_csv(args.filename)
duration = (df['time'].iloc[-1] - df['time'].iloc[0]).total_seconds()
print (f"Loaded {len(df)} samples from {args.filename} over {duration}s!")

dt = df['time'].diff().dt.total_seconds()[1:]
sample_rate = 1.0 / np.median(dt)

fig, axs = plt.subplots(3, 1, figsize=(20, 10), height_ratios=[2, 2, 1])
fig.canvas.manager.set_window_title("specgram")
fig.suptitle('Specgram of frequency', fontsize=16)
axs[0].specgram(df['acc'], Fs=sample_rate, NFFT=args.nfft, noverlap=32, cmap="plasma", detrend="mean")
axs[0].set_xlabel("time (s)")
axs[0].set_ylabel("freq (hz)")

bins = np.linspace(0, 0.1, 100)
axs[2].hist(dt, bins=bins)
axs[2].set_xlabel("dt")
axs[2].set_ylabel("count")

plt.show()
