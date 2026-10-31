# SS5000 front-panel LED control

Ubuntu can control the six dual-color front-panel LEDs through the Nuvoton
NCT6116 GPIO block:

- `power`
- `status`
- `info1`
- `info2`
- `info3`
- `info4`

The persistent controller is `/usr/local/sbin/ss5000-led`. Supported colors
are `off`, `blue`, `red`, and `purple`. Purple enables the blue and red
elements together.

## Commands

Show the configured and physical state:

```sh
sudo /usr/local/sbin/ss5000-led status
```

Set a solid color:

```sh
sudo /usr/local/sbin/ss5000-led set status blue
sudo /usr/local/sbin/ss5000-led set info1 red
sudo /usr/local/sbin/ss5000-led set info2 purple
```

Set a blinking color:

```sh
sudo /usr/local/sbin/ss5000-led set info3 blue --mode slow
sudo /usr/local/sbin/ss5000-led set info3 blue --mode normal
sudo /usr/local/sbin/ss5000-led set info3 blue --mode fast
```

The blink timings are:

- `slow`: 1.0 second on, 1.0 second off
- `normal`: 0.5 second on, 0.5 second off
- `fast`: 0.1 second on, 0.1 second off

Set all six LEDs or turn them off:

```sh
sudo /usr/local/sbin/ss5000-led all red
sudo /usr/local/sbin/ss5000-led all off
```

Run the flowing alarm pattern recovered from the original SAXA firmware:

```sh
# Original default: red, three-LED tail, one frame every 100 ms
sudo /usr/local/sbin/ss5000-led alarm start

# Optional color, tail length, and speed divisor
sudo /usr/local/sbin/ss5000-led alarm start blue --length 2 --speed 2
sudo /usr/local/sbin/ss5000-led alarm start purple --length 4 --speed 1

# Stop the alarm and restore the saved normal LED states
sudo /usr/local/sbin/ss5000-led alarm stop
```

The pattern flows in this order:

```text
INFO4 -> INFO3 -> INFO2 -> INFO1 -> STATUS -> POWER -> off
```

`--length` accepts 1 through 4 and controls the illuminated tail length.
`--speed` accepts 1 through 20 and is a divisor in the original driver's
sense: each frame lasts `100 ms * speed`, so a larger value moves more slowly.
Starting an alarm does not overwrite the six normal LED states. A normal
`set` or `all` command stops the alarm and applies the requested state.

Every modifying command saves the configuration, restarts the controller, and
waits until the first GPIO state has actually been written. The configuration
survives reboot in `/etc/ss5000/leds.json`.

The validated default leaves only the POWER LED solid blue:

```text
power: blue, solid
status: off, solid
info1: off, solid
info2: off, solid
info3: off, solid
info4: off, solid
```

## Hardware mapping and safety

The mapping recovered from the original SAXA `siogpio` driver is:

| LED | GPIO bank | Blue mask | Red mask |
| --- | ---: | ---: | ---: |
| POWER | 1 | `0x80` (active-low) | `0x40` |
| STATUS | 1 | `0x20` | `0x10` |
| INFO1 | 1 | `0x08` | `0x04` |
| INFO2 | 1 | `0x02` | `0x01` |
| INFO3 | 4 | `0x02` | `0x01` |
| INFO4 | 4 | `0x08` | `0x04` |

The privileged hardware helper checks the exact NCT6116 ID, verifies that the
GPIO logical device is enabled at the expected runtime base, serializes port
access, and rejects writes outside these LED masks. The raw helper is not
included in the sudo operator interface.

The helper exits Super I/O configuration mode with the same complete sequence
as the original SAXA driver: index-port `0xaa`, followed by global `CR02=02`.
This is required to avoid leaving the NCT6116 configuration interface unlocked
after LED updates, which can disturb other Super I/O functions such as UART.

`ss5000-led.service` is enabled and maintains blinking patterns. Members of
`ss5000-admin` may use the validated frontend through sudo.
