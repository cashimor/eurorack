# Responsibility and redistribution

**Maarten Hofman (cashimor) is not responsible for damage, loss, or malfunction of
Cre8audio gear or other equipment resulting from use of this project. Use it at
your own risk. There is no warranty or manufacturer endorsement.**

This project is independent of Cre8audio. Updating firmware can leave equipment
unusable, erase application data, or require external programming to recover.
Having a working USB connection is not proof that a flash operation is safe or
that the bootloader can recover every failure. The migration rewrites bootloader
flash; the application updater writes the application region.

The MIT license covers only our original source and documentation in this
directory. Protocol identifiers, register addresses, source-file fingerprints and
our observations are supplied to implement compatible tools. They are not a
distribution of the manufacturer's implementation.

Download official files from Cre8audio separately. Do not commit or redistribute
their firmware, updater applications, extracted executable code, disassemblies,
manuals, or vendor images through this project. A locally generated full Harmony
image includes the original vendor application and therefore also stays out of
the repository. The compiler and Python dependencies are obtained separately.

USB identifiers may be shared between products. Keep other Cre8audio devices
disconnected during an update. A driver tool can affect unrelated equipment if
the wrong device is selected; this project never changes drivers automatically.
