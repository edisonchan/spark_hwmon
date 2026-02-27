# SPBM - DGX Spark Power Telemetry hwmon Driver

Linux hwmon driver for the NVIDIA DGX Spark (GB10 SoC) that exposes
full system power telemetry via standard `sensors` / sysfs interfaces.

The driver reads the System Power Budget Manager (SPBM) shared memory,
which is continuously updated by the MediaTek SSPM firmware with live
power readings in milliwatts and cumulative energy counters in millijoules.

> **Note:** NVIDIA has [officially stated](https://forums.developer.nvidia.com/t/help-needed-how-to-enable-grace-cpu-power-telemetry-on-dgx-spark-gb10/360631)
> there is "no method to monitor CPU power" on the DGX Spark.

> [!WARNING]
> This driver is vibe coded. Thanks Claude.

## Sensors

23 power channels + 5 energy accumulators:

| Channel | Idle | Description |
|---------|------|-------------|
| sys_total | ~36 W | Total system power |
| soc_pkg | ~22 W | SoC package |
| cpu_gpu | ~10 W | CPU + GPU combined |
| cpu_p | ~4.5 W | P-core cluster (10x Cortex-X925) |
| cpu_e | ~0.1 W | E-core cluster (10x Cortex-A725) |
| vcore | ~4 W | Core voltage domain |
| dc_input | ~36 W | DC input / charger rail |
| gpu_out | ~4.8 W | GPU output (matches nvidia-smi) |
| prereg_in | ~8 W | Pre-regulator input |
| pl1 | 140 W | PL1 effective limit |
| syspl1 | 231 W | System PL1 effective limit |
| pkg | cumulative | Package energy (mJ) |
| cpu_e | cumulative | E-core energy (mJ) |
| cpu_p | cumulative | P-core energy (mJ) |

Under full load (all 20 cores): ~92 W package, ~64 W CPU_P, ~10.5 W CPU_E.

Energy accumulators are more accurate than instantaneous power readings
for computing averages (the firmware uses a 100 ms PID control loop that
causes instantaneous values to oscillate).

## Install via DKMS

```bash
sudo apt install dkms
sudo dkms add .
sudo dkms build spbm/0.1.0
sudo dkms install spbm/0.1.0
```

The module auto-loads at boot via ACPI modalias matching (`NVDA8800`).
DKMS automatically rebuilds the module on kernel updates.

## Manual Build

```bash
make            # build and sign (requires MOK keys for Secure Boot)
make load       # build, sign, load, show sensors
make unload     # unload module
make clean      # clean build artifacts
```

MOK signing keys are expected at `/var/lib/shim-signed/mok/MOK.{priv,der}`
(default Ubuntu location). Override with `MOK_KEY=` and `MOK_CERT=`.

## Requirements

- NVIDIA DGX Spark (GB10 SoC) with ACPI device `NVDA8800` (MTEL)
- Linux kernel 6.11+ (ARM64)
- Kernel headers (`linux-headers-$(uname -r)`)
- For Secure Boot: enrolled MOK key

## How It Works

The driver binds as an `acpi_driver` to the `NVDA8800` (MTEL) ACPI device
and extracts the SPBM shared memory address from the device's `_CRS`
resources. The MTEL device lacks `_UID` and `_STA` in the DSDT, so the
kernel never creates a `platform_device` for it — this is why a
`platform_driver` cannot be used.

The SPBM region was discovered by reverse-engineering the DSDT `_DSM`
method for `NVDA8800` (UUID `12345678-1234-1234-1234-123456789abc`).
No upstream Linux driver exists as of kernel 7.0.

## License

GPL-2.0
