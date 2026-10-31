# SS5000 internal L2 switch support

Validated hardware:

- PCI `0000:02:00.0`: Intel I210 (`8086:1538`)
- I210 MAC: `00:02:2b:c6:06:42`
- switch: RTL8367C, connected to the I210 over SGMII
- Ubuntu kernel: `7.0.0-14-generic`

Files:

- `igb-ss5000/igb.ko`: Linux v7.0 igb with the SS5000 SGMII changes
- `target-files/usr/local/libexec/ss5000-l2sw-init`: switch initialization
- `target-files/usr/local/sbin/ss5000-l2sw-config`: validated VLAN frontend
- `target-files/usr/local/sbin/ss5000-led`: validated front-panel LED frontend
- `target-files/usr/local/libexec/ss5000-led-hw`: restricted NCT6116 GPIO helper
- `target-files/usr/local/libexec/ss5000-mgmt-prime`: management-link monitor
- `target-files/usr/local/libexec/ss5000-mgmt-announce`: gratuitous ARP helper
- `target-files/etc/systemd/system/ss5000-l2sw.service`: boot ordering
- `target-files/etc/systemd/system/ss5000-led.service`: persistent LED control
- `target-files/etc/systemd/system/ss5000-mgmt-prime.service`: prime the ASIX
  management adapter on every carrier-up transition
- `target-files/etc/systemd/system/grub-initrd-fallback.service.d/ss5000-ordering.conf`:
  serialize the Ubuntu 26.04 GRUB environment writers
- `target-files/etc/ss5000/l2sw-vlans.json`: persistent switch VLAN layout
- `target-files/etc/ss5000/leds.json`: persistent LED layout
- `target-files/etc/netplan/70-ss5000-l2sw.yaml`: persistent `l2sw` name and
  default address `192.168.20.254/24`
- vendor `rtkctl`: installed on the target as
  `/usr/local/sbin/ss5000-rtkctl`

The configuration frontend creates tagged Linux interfaces as `l2v<VID>`.
Assign addresses or bridges to those interfaces according to the intended
server role. See `SS5000-L2SW-CONFIG.md`.

See `SS5000-LED-CONTROL.md` for LED names, colors, and blink modes.
See `SS5000-MGMT-NETWORK.md` for the management-port boot workaround.

The built module is tied to `7.0.0-14-generic`. A future kernel upgrade needs
the patch rebuilt against that kernel's headers before rebooting into it.

Rollback:

1. Disable `ss5000-l2sw.service`.
2. Restore the saved pre-configurable files from
   `/root/recovery/l2sw-configurable-v1/`, if working on the validated target.
3. Remove `/etc/netplan/70-ss5000-l2sw.yaml` if a full driver rollback is
   intended.
4. Remove
   `/lib/modules/7.0.0-14-generic/updates/ss5000/igb.ko`.
5. Run `depmod -a 7.0.0-14-generic` and regenerate the initramfs.
