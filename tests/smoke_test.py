#!/usr/bin/env python3
"""End-to-end smoke test for the wavelet compressor."""

import argparse
import struct
import subprocess
import tempfile
from pathlib import Path


def write_test_bmp(path: Path, width: int = 16, height: int = 16) -> None:
    row_size = (width * 3 + 3) & ~3
    pixels = bytearray()
    for y in range(height - 1, -1, -1):
        row = bytearray()
        for x in range(width):
            red = (x * 17) % 256
            green = (y * 17) % 256
            blue = ((x + y) * 9) % 256
            row.extend((blue, green, red))
        row.extend(b"\0" * (row_size - width * 3))
        pixels.extend(row)

    header = struct.pack(
        "<2sIHHI", b"BM", 54 + len(pixels), 0, 0, 54
    ) + struct.pack(
        "<IiiHHIIiiII", 40, width, height, 1, 24, 0,
        len(pixels), 2835, 2835, 0, 0
    )
    path.write_bytes(header + pixels)


def run(executable: Path, *args: Path | str, expect_success: bool = True) -> None:
    result = subprocess.run(
        [str(executable), *(str(arg) for arg in args)],
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if (result.returncode == 0) != expect_success:
        raise RuntimeError(
            f"unexpected exit code {result.returncode}: {' '.join(map(str, args))}\n"
            f"{result.stdout}"
        )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    args = parser.parse_args()
    executable = args.executable.resolve()

    with tempfile.TemporaryDirectory() as temporary_directory:
        work = Path(temporary_directory)
        source = work / "source.bmp"
        archive = work / "compressed.dat"
        restored = work / "restored.bmp"
        write_test_bmp(source)

        run(executable, "-compress", source, archive)
        run(executable, "-decompress", archive, restored)
        run(executable, "-compress", work / "missing.bmp", archive, expect_success=False)

        if not archive.is_file() or archive.stat().st_size == 0:
            raise RuntimeError("compressor did not create a non-empty archive")
        data = restored.read_bytes()
        if data[:2] != b"BM" or len(data) < 54:
            raise RuntimeError("decompressor did not create a valid BMP file")
        width, height = struct.unpack_from("<ii", data, 18)
        if (width, height) != (16, 16):
            raise RuntimeError(f"unexpected restored dimensions: {width}x{height}")

    print("Wavelet compression smoke test passed.")


if __name__ == "__main__":
    main()
