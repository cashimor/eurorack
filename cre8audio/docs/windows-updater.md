# Windows updater

**Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of
Cre8audio gear or other equipment resulting from use of this project. Use it at
your own risk. There is no warranty or manufacturer endorsement.**

These instructions target **Cellz**. The legacy migration was exercised on a spare
whose old application reported `001b` and whose old loader reported `0003`.
The new loader reports `0102`. Other old revisions are deliberately rejected.
An existing new HID loader needs only the application-update steps.

## Set up Python and obtain the official files

Use Python 3.11 or later on Windows. In the `cre8audio` directory:

```powershell
python -m venv .venv
.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
python tools/cellz_update.py inspect
```

If PowerShell blocks activation, invoke `.venv\Scripts\python.exe` directly instead
of changing your system's execution policy. Download and extract the official
package from [Cre8audio](https://www.cre8audio.com/firmware-and-software) into the
ignored `vendor-input` directory, or keep it outside the repository.

Supported official fingerprints:

| File | SHA-256 |
| --- | --- |
| `Cellz_V0.0.3.0_220407A.dfu` | `f40a490705a33417e06fcf7b80f7eeb8bb78ebb6fbe894d5e3385b08d88e4ac9` |
| `NiftySeries_boot_install.dfu` | `49a821eddd00c41f41980606ebeab708bef21b251ba142b0c9743034c0820492` |

No vendor download is bundled. If your files differ, do not bypass the checks.

## USB drivers

The new `Nifty Series Upgrade` loader, VID:PID **1ACC:1A59**, works with Windows'
native HID driver. An old Cellz at **1ACC:1A4D** may need WinUSB for vendor-control
access. A previous WinUSB binding at 1A4D can also remain usable for the newer
application through our raw HID-class fallback.

For an old device requiring a driver change, obtain [Zadig](https://zadig.akeo.ie/)
separately. Select **Cre8audioCellz**, verify **1ACC:1A4D**, and bind **WinUSB**.
Do not replace the driver of a printer, another module, or the new 1A59 loader.
These tools do not perform driver changes themselves.

## One-time legacy migration, without a Mac

Skip this section if `inspect` identifies `Nifty Series Upgrade`, revision `0102`,
or the normal V0.0.3.0 application with a working HID status report.

1. Connect only the intended Cellz and keep its power stable. Start with its old
   functioning application, not an unidentified device already called Nifty Series.
2. Plan the migration using your separately downloaded installer:

   ```powershell
   python tools/cellz_update.py migrate --installer "vendor-input/NiftyBUNDLE Firmware/Step1 - Bootloader/NiftySeries_boot_install.dfu"
   ```

3. Add `--execute` to perform it. The tool enters the old loader, uploads the
   pinned installer, verifies its acknowledgments and starts it. **Starting that
   installer causes the device to attempt a bootloader rewrite.**
4. The expected intermediate product is **Cre8audio boot updata** (manufacturer's
   spelling), revision `0001`. Its appearance does not prove a successful rewrite.
5. Use the physical switch beside the USB connector to enter the new loader. On
   the tested unit, pressing the switch before connecting USB lit a red PCB LED
   and enumerated **Nifty Series Upgrade**, `1ACC:1A59`, revision `0102`.
6. Run `inspect` to confirm the loader, then install the official application below.

The exact switch hold duration is not established for every board. If necessary,
hold the switch while connecting USB and release once the loader appears. Do not
short unidentified pads. Software reset from the migration installer returned to
the installer in our trial, so the tool does not repeat that failed handoff.
If the loader does not appear, stop and seek a recovery procedure for your board.
There is no automatic rollback or flash backup.

If disconnecting USB causes your PC to shut down, resolve that behavior before
attempting physical reconnection procedures. Keep USB and case power connected
throughout an actual transfer. An unexpected power LED on an otherwise unpowered
rail should be investigated before more combined-power testing.

## Install or restore the official application

With the new loader available, or the supported V0.0.3.0 application running:

```powershell
python tools/cellz_update.py flash --firmware "vendor-input/NiftyBUNDLE Firmware/Step2 - Firmware/Cellz_V0.0.3.0_220407A.dfu"
```

Review the offline plan, then add `--execute`. The tool requests loader entry when
the supported application is running, stages and reads back each block, commits
once, checks checksum `3a9a`, and only then requests application return. It verifies
the fresh application status, not just a possibly cached USB descriptor.

The same command restores the official application from Harmony P1. It leaves the
new bootloader in place. Restoration has no promise of preserving programmed notes.

## Custom Harmony firmware

Follow [the local build instructions](harmony.md). Supply both the generated DFU
and its generated manifest; add `--execute` only when ready to install:

```powershell
python tools/cellz_update.py flash --firmware build/harmony/Cellz_Harmony_P1.UNTESTED.dfu --manifest build/harmony/build.json
```

The manifest ties the source fingerprint, full image hash and container hash
together. Only use manifests from builds you trust. The updater also requires the
`HARMONY1` application signature after starting this prototype.

Local journals go into ignored `logs/`. A failure stops without replaying commits
or forcing a partially written application to start. Preserve the journal when
diagnosing a problem; review it before sharing. These commands do not read back
flash: RAM readback and the loader's whole-image check are different verifications.
