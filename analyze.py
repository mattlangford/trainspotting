import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import argparse

def load_csv(fname):
    df = pd.read_csv(fname)
    df["time"] =  pd.to_datetime(df['time'], errors='coerce', unit='ms')
    return df

parser = argparse.ArgumentParser()
parser.add_argument("filename")
parser.add_argument("-n", "--nfft", default=256)
args = parser.parse_args()

imu_df = load_csv(args.filename)
duration = (imu_df['time'].iloc[-1] - imu_df['time'].iloc[0]).total_seconds()
print (f"Loaded {len(imu_df)} samples from {args.filename} over {duration}s!")

dt = imu_df['time'].diff().median().total_seconds()
sample_rate = 1.0 / dt
print(f"Sample rate: {sample_rate:.3f}hz")


fig, axs = plt.subplots(2, 1, figsize=(20, 10), height_ratios=[3, 1])
fig.canvas.manager.set_window_title("specgram")
fig.suptitle('Specgram of frequency', fontsize=16)
axs[0].specgram(np.sqrt(imu_df['mag2']), Fs=sample_rate, NFFT=args.nfft, noverlap=32, cmap="plasma", detrend="mean")
axs[0].set_xlabel("time (s)")
axs[0].set_ylabel("freq (hz)")

bins = np.linspace(0, 0.1, 100)
axs[1].hist(imu_df['time'].diff().dt.total_seconds(), bins=bins)
axs[1].set_xlabel("dt")
axs[1].set_ylabel("count")

plt.show()
