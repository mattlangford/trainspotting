import numpy as np
import pandas as pd
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.ticker import FormatStrFormatter
import matplotlib.mlab
import argparse
import scipy.stats
import scipy.signal
import datetime

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
parser.add_argument("--max-power", default=-40, type=int)
parser.add_argument("--min-power", default=-80, type=int)
parser.add_argument("--start", default=None, type=str)
parser.add_argument("--trim", default=1, type=int)
args = parser.parse_args()

df = load_csv(args.filename)
df = df[args.trim:-args.trim]

duration = (df['time'].iloc[-1] - df['time'].iloc[0]).total_seconds()
print (f"Loaded {len(df)} samples from {args.filename} over {duration}s!")

dt = df['time'].diff().dt.total_seconds()[1:]
sample_rate = 1.0 / np.median(dt)

fig, axs = plt.subplots(2, 1, figsize=(12, 8), height_ratios=[3, 1], sharex=True)
fig.canvas.manager.set_window_title("specgram")
fig.suptitle('Specgram of frequency', fontsize=16)

Pxx, freqs, bins, im = axs[0].specgram(
    df['acc'],
    Fs=sample_rate,
    NFFT=args.nfft,
    noverlap=args.overlap,
    cmap="plasma",
    detrend="mean",
    window=matplotlib.mlab.window_none,
    vmin=args.min_power,
    vmax=args.max_power
)
print(f"Min Power: {10 * np.log10(max(Pxx.min(), 1E-10)):.3f}")
print(f"Max Power: {10 * np.log10(Pxx.max()):.3f}")

axs[0].set_ylabel("freq (hz)")

if args.start:
    start_time = pd.to_datetime(args.start)
    fmt = lambda x, pos: '{}\n{}'.format(x, start_time + datetime.timedelta(seconds=x))
    axs[0].xaxis.set_major_formatter(mpl.ticker.FuncFormatter(fmt))

# num_bins = len(bins)
# step = max(1, int(args.x_ticks * 60 / sample_rate))
# axs[0].set_xticks(bins[::step])
# times = df['time'].iloc[0] + pd.to_timedelta(bins[::step], unit="s")
# axs[0].set_xticklabels([f"{time.strftime('%m-%d %H:%M:%S')}" for time in times], rotation=45, ha='right')

axs[1].plot(bins, 10 * np.log10(np.sum(Pxx, axis=0) / (Pxx.shape[1])))
axs[1].set_xlabel("time (s)")
axs[1].set_ylabel("power (dB)")

fig.tight_layout()
fig.subplots_adjust(left=0.10, right=0.95, top=0.95, bottom=0.10)


plt.show()
