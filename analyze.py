import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

def load_csv(fname):
    df = pd.read_csv(fname)
    df["total"] = np.linalg.norm(df[['x','y','z']].values, axis=1)
    return df


imu = load_csv("/Volumes/data/IMU_468.csv")
plt.plot(imu["time"], imu["total"], label='total', color='k')
plt.legend()
plt.show()