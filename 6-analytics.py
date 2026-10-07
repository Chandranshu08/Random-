"""
INDUSTRIAL TEMPERATURE DATA ANALYTICS (VS Code, Python 3)
=========================================================
SETUP
  1. pip install pandas numpy matplotlib seaborn
  2. Put the dataset CSV in the same folder (any name).
  3. Run:   python analytics.py yourfile.csv      (or edit FILE below and press Run)
  4. Plots are shown AND saved as PNG in the folder "plots".

WHAT IT DOES (matches the lab steps)
  load -> preprocess (-200/blank = missing, duplicates, date parsing) -> statistics ->
  trend + rolling mean -> distribution -> box plot -> hourly/daily pattern ->
  correlation heatmap -> IQR outliers -> printed insights.

If the temperature column is not auto-detected, set TEMP_COL = "exact_column_name" below.
"""
import os, sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

FILE = "data.csv"
TEMP_COL = None            # e.g. "Temperature" ; None = auto-detect

path = sys.argv[1] if len(sys.argv) > 1 else FILE
os.makedirs("plots", exist_ok=True)

# ---------- 1. LOAD ----------
df = pd.read_csv(path, sep=None, engine="python")      # auto-detects , ; or tab
df = df.dropna(how="all", axis=1).dropna(how="all")
print("Shape:", df.shape)
print(df.head(), "\n")

# ---------- 2. PREPROCESS ----------
df.replace([-200, -999, "NA", "N/A", "null"], np.nan, inplace=True)
for c in df.columns:                                    # convert text numbers (also 1,5 -> 1.5)
    if df[c].dtype == object:
        t = pd.to_numeric(df[c].astype(str).str.replace(",", ".", regex=False), errors="coerce")
        if t.notna().mean() > 0.8:
            df[c] = t

# datetime column (Date + Time, or any column with date/time/stamp in its name)
cols_lower = {c.lower(): c for c in df.columns}
if "date" in cols_lower and "time" in cols_lower:
    df["Datetime"] = pd.to_datetime(df[cols_lower["date"]].astype(str) + " " +
                                    df[cols_lower["time"]].astype(str).str.replace(".", ":", regex=False),
                                    dayfirst=True, errors="coerce")
else:
    cand = [c for c in df.columns if any(k in c.lower() for k in ("date", "time", "stamp"))]
    df["Datetime"] = pd.to_datetime(df[cand[0]], dayfirst=True, errors="coerce") if cand else pd.NaT
has_time = df["Datetime"].notna().sum() > 0

dups = df.duplicated().sum()
df = df.drop_duplicates()
print("Duplicates removed:", dups)
print("Missing values per column:\n", df.isna().sum(), "\n")

num = df.select_dtypes(include=np.number)
if TEMP_COL is None:
    named = [c for c in num.columns if "temp" in c.lower()]
    TEMP_COL = named[0] if named else num.columns[0]
print("Analysing column:", TEMP_COL)

if has_time:
    df = df.sort_values("Datetime")
df[num.columns] = df[num.columns].interpolate(limit_direction="both")   # fill gaps
num = df.select_dtypes(include=np.number)
t = df[TEMP_COL]

# ---------- 3. STATISTICS ----------
print("\nSTATISTICAL SUMMARY\n", num.describe().round(2))
print("\nMean %.2f | Median %.2f | Std %.2f | Min %.2f | Max %.2f" %
      (t.mean(), t.median(), t.std(), t.min(), t.max()))

def save(name):
    plt.tight_layout(); plt.savefig(f"plots/{name}.png", dpi=120); plt.show()

x = df["Datetime"] if has_time else df.index

# ---------- 4. TREND ----------
plt.figure(figsize=(10, 4))
plt.plot(x, t, alpha=0.4, label="Raw")
plt.plot(x, t.rolling(24, min_periods=1).mean(), color="red", label="Rolling mean (24)")
plt.title(f"{TEMP_COL} Trend"); plt.xlabel("Time"); plt.ylabel(TEMP_COL); plt.legend(); plt.grid(alpha=.3)
save("1_trend")

# ---------- 5. DISTRIBUTION ----------
plt.figure(figsize=(6, 4)); sns.histplot(t, bins=30, kde=True, color="teal")
plt.title(f"{TEMP_COL} Distribution"); save("2_histogram")

# ---------- 6. BOX PLOT (outliers) ----------
plt.figure(figsize=(8, 4)); num.plot(kind="box", ax=plt.gca(), rot=45)
plt.title("Box Plot of Numeric Columns"); save("3_boxplot")

# ---------- 7. HOURLY / DAILY PATTERN ----------
if has_time:
    df["Hour"] = df["Datetime"].dt.hour
    df["Day"] = df["Datetime"].dt.day_name()
    df.groupby("Hour")[TEMP_COL].mean().plot(marker="o", figsize=(8, 4), title="Average by Hour of Day")
    plt.xlabel("Hour"); plt.ylabel(TEMP_COL); plt.grid(alpha=.3); save("4_hourly")
    order = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]
    df.groupby("Day")[TEMP_COL].mean().reindex(order).plot(marker="o", figsize=(8, 4), title="Average by Day of Week")
    plt.ylabel(TEMP_COL); plt.grid(alpha=.3); save("5_weekly")

# ---------- 8. CORRELATION ----------
if num.shape[1] > 1:
    plt.figure(figsize=(8, 6)); sns.heatmap(num.corr(), annot=True, fmt=".2f", cmap="coolwarm")
    plt.title("Correlation Heatmap"); save("6_correlation")
    print("\nCorrelation of other columns with", TEMP_COL)
    print(num.corr()[TEMP_COL].drop(TEMP_COL).sort_values(ascending=False).round(2))

# ---------- 9. OUTLIERS (IQR) ----------
q1, q3 = t.quantile(.25), t.quantile(.75)
iqr = q3 - q1
low, high = q1 - 1.5 * iqr, q3 + 1.5 * iqr
out = (t < low) | (t > high)
print(f"\nIQR method: Q1={q1:.2f} Q3={q3:.2f} bounds=({low:.2f}, {high:.2f}) -> {out.sum()} outliers")
plt.figure(figsize=(10, 4))
plt.plot(x, t, alpha=0.5)
plt.scatter(np.array(x)[out.values], t[out], color="red", s=12, label="Outliers")
plt.title("Anomalies (IQR)"); plt.legend(); plt.grid(alpha=.3); save("7_outliers")

# ---------- 10. INSIGHTS ----------
print("\nINSIGHTS")
print(f"- Average {TEMP_COL} = {t.mean():.2f}, range {t.min():.2f} to {t.max():.2f}.")
print(f"- {out.sum()} readings lie outside the IQR bounds (possible overheating events or sensor faults - check against the process).")
if has_time:
    h = df.groupby('Hour')[TEMP_COL].mean()
    print(f"- Hottest hour of the day: {int(h.idxmax())}:00 ({h.max():.2f}); coolest: {int(h.idxmin())}:00 ({h.min():.2f}).")
print("- Outlier does not automatically mean error; it may be a real unusual event.")
