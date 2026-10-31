#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/io.h>
#include <unistd.h>

#define SIO_CONFIG_PORT 0x2e
#define EXPECTED_CHIP_ID 0xd282
#define GPIO_LDN 0x08
#define LOCK_PATH "/run/lock/ss5000-led-hw.lock"

static void die(const char *message)
{
    fprintf(stderr, "ss5000-led-hw: %s\n", message);
    exit(EXIT_FAILURE);
}

static void die_errno(const char *message)
{
    fprintf(stderr, "ss5000-led-hw: %s: %s\n", message, strerror(errno));
    exit(EXIT_FAILURE);
}

static void sio_write(unsigned char reg, unsigned char value)
{
    outb(reg, SIO_CONFIG_PORT);
    outb(value, SIO_CONFIG_PORT + 1);
}

static unsigned char sio_read(unsigned char reg)
{
    outb(reg, SIO_CONFIG_PORT);
    return inb(SIO_CONFIG_PORT + 1);
}

/*
 * Match the original SAXA siogpio driver's complete Super I/O exit sequence.
 * The 0xaa write closes Nuvoton configuration mode; CR02=02 is the additional
 * global lock used by the vendor driver.  Omitting 0xaa can leave the chip in
 * configuration mode after an LED update and disturb other Super I/O users.
 */
static void sio_exit(void)
{
    outb(0xaa, SIO_CONFIG_PORT);
    sio_write(0x02, 0x02);
}

static unsigned short detect_gpio_base(void)
{
    unsigned short chip_id;
    unsigned short base;
    unsigned char active;

    if (ioperm(SIO_CONFIG_PORT, 2, 1) != 0)
        die_errno("cannot access Super I/O configuration ports");

    outb(0x87, SIO_CONFIG_PORT);
    outb(0x87, SIO_CONFIG_PORT);

    chip_id = ((unsigned short)sio_read(0x20) << 8) | sio_read(0x21);
    if (chip_id != EXPECTED_CHIP_ID) {
        sio_exit();
        fprintf(stderr, "ss5000-led-hw: unexpected Super I/O ID 0x%04x\n",
                chip_id);
        exit(EXIT_FAILURE);
    }

    sio_write(0x07, GPIO_LDN);
    active = sio_read(0x30);
    base = ((unsigned short)sio_read(0x60) << 8) | sio_read(0x61);
    sio_exit();

    if ((active & 0x02) == 0)
        die("NCT6116 GPIO logical device is disabled");
    if (base < 0x100 || base > 0xfff)
        die("NCT6116 GPIO base address is invalid");
    if (ioperm(base, 5, 1) != 0)
        die_errno("cannot access NCT6116 GPIO ports");
    return base;
}

static unsigned char gpio_read(unsigned short base, unsigned char bank)
{
    outb(bank, base);
    return inb(base + 2);
}

static void gpio_write(unsigned short base, unsigned char bank,
                       unsigned char value)
{
    outb(bank, base);
    outb(value, base + 2);
}

static unsigned long parse_number(const char *text, const char *label)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') {
        fprintf(stderr, "ss5000-led-hw: invalid %s: %s\n", label, text);
        exit(EXIT_FAILURE);
    }
    return value;
}

int main(int argc, char **argv)
{
    unsigned short base;
    int lock_fd;

    if (geteuid() != 0)
        die("root privileges are required");

    lock_fd = open(LOCK_PATH, O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (lock_fd < 0)
        die_errno("cannot open lock file");
    if (flock(lock_fd, LOCK_EX) != 0)
        die_errno("cannot lock GPIO access");

    base = detect_gpio_base();

    if (argc == 2 && strcmp(argv[1], "snapshot") == 0) {
        printf("bank1=0x%02x bank4=0x%02x\n",
               gpio_read(base, 1), gpio_read(base, 4));
        return EXIT_SUCCESS;
    }

    if (argc == 5 && strcmp(argv[1], "write") == 0) {
        unsigned long bank_value = parse_number(argv[2], "bank");
        unsigned long mask_value = parse_number(argv[3], "mask");
        unsigned long requested = parse_number(argv[4], "value");
        unsigned char allowed;
        unsigned char previous;
        unsigned char updated;
        unsigned char verified;

        if (bank_value != 1 && bank_value != 4)
            die("only LED banks 1 and 4 are allowed");
        allowed = bank_value == 1 ? 0xff : 0x0f;
        if (mask_value > 0xff || requested > 0xff ||
            (mask_value & ~allowed) != 0)
            die("write touches a non-LED GPIO bit");

        previous = gpio_read(base, (unsigned char)bank_value);
        updated = (previous & ~(unsigned char)mask_value) |
                  ((unsigned char)requested & (unsigned char)mask_value);
        gpio_write(base, (unsigned char)bank_value, updated);
        verified = gpio_read(base, (unsigned char)bank_value);
        if (verified != updated)
            die("GPIO write verification failed");
        printf("bank%lu: 0x%02x -> 0x%02x\n",
               bank_value, previous, updated);
        return EXIT_SUCCESS;
    }

    fprintf(stderr,
            "Usage: %s snapshot\n"
            "       %s write <1|4> <mask> <value>\n",
            argv[0], argv[0]);
    return EXIT_FAILURE;
}
