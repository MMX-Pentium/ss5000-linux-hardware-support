#!/usr/bin/env python3
"""Create a test-only flat-VLAN variant of the saved SS5000 rtkctl."""

from pathlib import Path
import argparse
import hashlib


SOURCE_SHA256 = "3454ceccf0aa9a094d2db6a37756759605d4119dc3fb6e945e11d87172fbea68"


def patch(data: bytearray, offset: int, expected: bytes, replacement: bytes) -> None:
    actual = bytes(data[offset : offset + len(expected)])
    if actual != expected:
        raise RuntimeError(
            f"offset 0x{offset:x}: expected {expected.hex()}, found {actual.hex()}"
        )
    if len(expected) != len(replacement):
        raise ValueError("in-place patches must keep their original length")
    data[offset : offset + len(replacement)] = replacement


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument(
        "--cpu-untag",
        action="store_true",
        help="also make EXT_PORT0 untagged, for direct use of the Linux parent NIC",
    )
    parser.add_argument(
        "--exact-vendor-ext",
        action="store_true",
        help="match rtl8367_ext_port_up by disabling EXT_PORT0 in-band negotiation",
    )
    args = parser.parse_args()

    source = args.source
    output = args.output
    raw = source.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != SOURCE_SHA256:
        raise RuntimeError(f"unexpected source SHA-256: {digest}")

    data = bytearray(raw)

    # The standalone rtkctl "up" command requests nway=1, while the original
    # rtl8367_ext_port_up kernel helper passed nway=0.  The latter is the exact
    # configuration used when the stock igb driver probed this hardware.
    if args.exact_vendor_ext:
        patch(data, 0x18B4, b"\x01\x00\x00\x00", b"\x00\x00\x00\x00")

    # Reuse the existing "setvlan" command as a test-only flat VLAN 1 setup.
    # VLAN 1 member mask: physical ports 0..4 plus EXT_PORT0 (bit 16).
    # Untag mask: physical ports 0..4.  With --cpu-untag, EXT_PORT0 is also
    # untagged so Linux receives traffic directly through the parent NIC.
    patch(data, 0x19A5, b"\x03\x00\x00\x00", b"\x01\x00\x00\x00")
    patch(data, 0x19C0, b"\x01\x00\x01\x00", b"\x1f\x00\x01\x00")
    untag_mask = b"\x1f\x00\x01\x00" if args.cpu_untag else b"\x1f\x00\x00\x00"
    patch(data, 0x19C8, b"\x01\x00\x00\x00", untag_mask)

    # Suppress the four later rtk_vlan_set calls for VLANs 4 through 7.
    for offset, expected in (
        (0x19F9, b"\xe8\x32\x61\x02\x00"),
        (0x1A26, b"\xe8\x05\x61\x02\x00"),
        (0x1A53, b"\xe8\xd8\x60\x02\x00"),
        (0x1A80, b"\xe8\xab\x60\x02\x00"),
    ):
        patch(data, offset, expected, b"\x90" * 5)

    # Assign untagged ingress on all five physical ports to VLAN 1.
    for offset, original_vid in (
        (0x1A88, 3),
        (0x1A96, 4),
        (0x1AA7, 5),
        (0x1AB8, 6),
        (0x1AC9, 7),
    ):
        patch(
            data,
            offset,
            original_vid.to_bytes(4, "little"),
            (1).to_bytes(4, "little"),
        )

    output.write_bytes(data)
    output.chmod(0o755)
    print(hashlib.sha256(data).hexdigest(), output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
