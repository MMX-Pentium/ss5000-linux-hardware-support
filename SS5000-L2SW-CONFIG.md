# SS5000 L2SW VLAN configuration

The RTL8367C switch can be configured without rebuilding a firmware image.
The supported model is:

- physical LAN3 through LAN7 are 802.1Q access ports;
- each physical port has one VLAN ID in the range 1 through 4094;
- ports assigned the same VLAN can switch frames with each other;
- the CPU/I210 uplink is a member of every configured VLAN;
- zero or one CPU VLAN can be native/untagged;
- every other CPU VLAN appears in Linux as `l2v<VID>`;
- up to five distinct VLAN IDs can be active.

Physical-port trunk mode is not implemented. Frames on LAN3 through LAN7 are
untagged on egress and classified by that port's configured PVID on ingress.

## Current default

All five ports are in VLAN 20 and the CPU side is untagged:

```text
VLAN 20: lan3,lan4,lan5,lan6,lan7 cpu=native
```

Linux uses the parent interface `l2sw`, persistently configured as
`192.168.20.254/24`.

Show the saved and active state:

```sh
sudo /usr/local/sbin/ss5000-l2sw-config show
sudo /usr/local/sbin/ss5000-l2sw-config status
```

## Change VLANs

The following example moves LAN7 to VLAN 100. VLAN 20 remains native on the
CPU, so Linux creates the tagged interface `l2v100`:

```sh
sudo /usr/local/sbin/ss5000-l2sw-config set lan7 100
sudo /usr/local/sbin/ss5000-l2sw-config show
sudo /usr/local/sbin/ss5000-l2sw-config apply
```

The save and apply can also be combined:

```sh
sudo /usr/local/sbin/ss5000-l2sw-config set lan7 100 --apply
```

To make VLAN 100 the CPU native VLAN, it must first be assigned to at least
one physical port:

```sh
sudo /usr/local/sbin/ss5000-l2sw-config set lan7 100
sudo /usr/local/sbin/ss5000-l2sw-config native 100
sudo /usr/local/sbin/ss5000-l2sw-config apply
```

For a fully tagged CPU uplink:

```sh
sudo /usr/local/sbin/ss5000-l2sw-config native tagged --apply
```

With a native VLAN, use `l2sw` for that VLAN. For tagged CPU VLANs, use the
automatically created interfaces such as `l2v100`. The VLAN tool creates and
raises these links but does not assign their IP addresses or bridges.

The source JSON is `/etc/ss5000/l2sw-vlans.json`. Direct editing is supported,
but validate before applying:

```sh
sudo /usr/local/sbin/ss5000-l2sw-config validate
sudo /usr/local/sbin/ss5000-l2sw-config apply
```

## Rollback and safety

An apply briefly resets the switch-facing I210. Use `mgmt0` for the SSH
session while changing VLANs and allow several seconds for physical link and
ARP learning to settle.

On a service failure, the CLI automatically reapplies the last successful
configuration. To manually restore the preceding successful configuration:

```sh
sudo /usr/local/sbin/ss5000-l2sw-config rollback
```

The persistent service is `ss5000-l2sw.service`. Its boot-time state and the
last successful configuration are shown by `status`.

Members of `ss5000-admin` may run the configuration frontend through sudo.
The installed `ss5000` account has been added to that group; a new login is
required before a pre-existing session sees the supplementary group.

