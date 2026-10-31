#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only

PATH=/usr/sbin:/usr/bin:/sbin:/bin
export PATH

sio=/usr/sbin/sio

emit()
{
    message="SS5000-WDT: $*"
    printf '%s\n' "$message" >/dev/console
    printf '<6>%s\n' "$message" >/dev/kmsg
}

if [ ! -x "$sio" ]; then
    emit "FAIL: $sio is missing from dracut initramfs"
    return 1
fi

attempt=1
while [ "$attempt" -le 5 ]; do
    stop_output=$("$sio" -d 8 30=2 2>&1)
    stop_status=$?
    registers=$("$sio" -d 8 30 f0 f1 f2 2>&1)

    if [ "$stop_status" -eq 0 ] &&
        printf '%s\n' "$registers" | grep -q 'R 0x30 0x02'; then
        emit "SUCCESS: NCT611xD watchdog disabled by dracut hook: $registers"
        return 0
    fi

    emit "retry $attempt: rc=$stop_status output=[$stop_output] registers=[$registers]"
    attempt=$((attempt + 1))
    sleep 1
done

diagnostic=$("$sio" -v -d 8 30 f0 f1 f2 2>&1)
emit "FAIL: dracut watchdog disable not verified; diagnostic=[$diagnostic]"
return 1
