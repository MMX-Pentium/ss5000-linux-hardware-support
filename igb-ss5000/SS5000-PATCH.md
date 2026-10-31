# SS5000 I210/RTL8367C compatibility patch

The SS5000 connects PCI `0000:02:00.0` (Intel I210, device `8086:1538`) to
an RTL8367C L2 switch over SGMII/MDIO. The Ubuntu 26.04 Linux 7.0 `igb`
driver treats this as a conventional copper PHY and fails probe with
`error -2`.

The original SAXA `3.16.0-4-saxa` `igb.ko` was compared with its unmodified
Debian counterpart. In the SGMII/MDIO branch it:

1. selects `e1000_media_type_internal_serdes`;
2. marks SGMII active; and
3. calls the vendor `rtl8367_ext_port_up` helper.

This source keeps the first two behaviors. Switch initialization is performed
separately by the saved vendor `rtkctl` utility, which directly accesses the
I210 MDIO registers from user space.

The RTL8367C CPU port is forced to 1000/full with SGMII in-band negotiation
disabled. Modern upstream `igb` enables PCS autoneg for SGMII, which reaches
symbol synchronization but not `PCS_LSTS.LINK_OK` on this hardware. This
source therefore also restores the forced-PCS behavior found in SAXA's
3.16 driver.

Upstream base: Linux tag `v7.0`, commit
`028ef9c96e96197026887c0f092424679298aae8`.
