# SS5000 management-port boot workaround

The management port is the USB ASIX AX88772B adapter:

- interface: `mgmt0`
- permanent MAC: `00:0e:c6:87:72:01`
- default address: `10.1.5.254/24`

On the validated device, the first management-side transmission after a reboot
can be required before the attached PC can reach the appliance.  The
`ss5000-mgmt-prime.service` link monitor makes that transition automatic:

1. wait for `mgmt0` carrier and its networkd IPv4 configuration;
2. on every DOWN-to-UP transition, toggle the AX88772B receive-mode
   programming once;
3. broadcast gratuitous ARP requests and replies;
4. issue a best-effort probe to the configured directly connected peer;
5. repeat the gratuitous ARP announcement.

The default peer is the validation PC at `10.1.5.2`.  Change or clear
`PEER_IPV4` in `/etc/default/ss5000-mgmt-prime` for another deployment.

Status and logs:

```sh
systemctl status ss5000-mgmt-prime.service
journalctl -b -u ss5000-mgmt-prime.service
```

The monitor remains idle when the cable or management adapter is absent, so it
does not hold up or fail a headless boot.  Plugging the cable in later triggers
the same repair sequence.
