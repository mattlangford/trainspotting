import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import scipy.signal
from scipy.signal import square, ShortTimeFFT
from scipy.signal.windows import gaussian

def load_csv(fname):
    df = pd.read_csv(fname)
    df["time"] =  pd.to_datetime(df['time'], errors='coerce', unit='ms')
    df["total"] = np.linalg.norm(df[['x','y','z']].values, axis=1)
    return df


imu_df = load_csv("/Volumes/data/IMU_468.csv")
# plt.plot(imu["time"], imu["total"], label='total', color='k')
# plt.legend()
# plt.show()
imu_groups = imu_df.groupby(imu_df['time'].dt.second)

g_std = 3
win = scipy.signal.windows.gaussian(5, std=g_std, sym=True)
SFT = ShortTimeFFT(win, hop=2, fs=1/T_x, mfft=800, scale_to='psd')

scipy.signal.spectrogram(imu_df['time'], imu_df['total'])