#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <ftdi.h>
#include <usb.h>
#include <sys/ioctl.h>

#define NAME			program_invocation_short_name

#define CBUS_0_IN_MASK		0x01
#define CBUS_3_OUT_LOW		0x80
#define CBUS_3_OUT_HIGH		0x88

#define IOCTL_USB_IOCTL		_IOWR('U', 18, struct usb_ioctl)
#define IOCTL_USB_CONNECT	_IO('U', 23)

struct usb_ioctl {
	int ifno;
	int ioctl_code;
	void *data;
};

struct usb_dev_handle
{
	int fd;
};

int usb_attach_kernel_driver_np(struct usb_dev_handle *dev, int interface)
{
	struct usb_ioctl command;

	command.ifno = interface;
	command.ioctl_code = IOCTL_USB_CONNECT;
	command.data = NULL;

	return ioctl(dev->fd, IOCTL_USB_IOCTL, &command);
}

int modem_is_on(struct ftdi_context *ftdic) {
	unsigned char data;
	int ret;

	ret = ftdi_read_pins(ftdic, &data);
	if (ret < 0)
		return -1;

	return data & CBUS_0_IN_MASK;
}

void usage(void)
{
	fprintf(stderr, "usage: %s [on | off]\n", NAME);
	exit(EXIT_FAILURE);
}

/*
 * Main
 */

int main(int argc, char **argv)
{
	struct ftdi_context ftdic;
	int force_status = -1;
	int ret;

	if (argc == 2) {
		if (strcmp(argv[1], "on") == 0)
			force_status = 1;
		else if (strcmp(argv[1], "off") == 0)
			force_status = 0;
		else
			usage();
	}

	ret = ftdi_init(&ftdic);
	if (ret < 0) {
		fprintf(stderr, "ftdi_init failed\n");
		return EXIT_FAILURE;
	}

	ret = ftdi_usb_open(&ftdic, 0x0403, 0x6015);
	if (ret < 0) {
		fprintf(stderr, "Can't open ftdi device\n");
		return EXIT_FAILURE;
	}

	ret = ftdi_set_bitmode(&ftdic, CBUS_3_OUT_HIGH, BITMODE_CBUS);
	if (ret < 0) {
		fprintf(stderr, "unable to set low: %s\n",
			ftdi_get_error_string(&ftdic));
		return EXIT_FAILURE;
	}

	ret = modem_is_on(&ftdic);
	if (ret < 0) {
		fprintf(stderr, "unable to read modem status: %s\n",
			ftdi_get_error_string(&ftdic));
		return EXIT_FAILURE;
	}

	if ((force_status >= 0) && (force_status ^ ret)) {
		/* Force status changing! */
		ret = ftdi_set_bitmode(&ftdic, CBUS_3_OUT_LOW, BITMODE_CBUS);
		if (ret < 0) {
			fprintf(stderr, "unable to set low: %s\n",
				ftdi_get_error_string(&ftdic));
			return EXIT_FAILURE;
		}

		usleep(2000000);	/* 2s */
	
		ret = ftdi_set_bitmode(&ftdic, CBUS_3_OUT_HIGH, BITMODE_CBUS);
		if (ret < 0) {
			fprintf(stderr, "unable to set high: %s\n",
				ftdi_get_error_string(&ftdic));
			return EXIT_FAILURE;
		}
	
		usleep(2000000);	/* 2s */
	
		ret = modem_is_on(&ftdic);
		if (ret < 0) {
			fprintf(stderr, "unable to read modem status: %s\n",
				ftdi_get_error_string(&ftdic));
			return EXIT_FAILURE;
		}
	}
	printf("modem is %s\n", ret ? "on" : "off");

#ifdef LIBFTDI_LINUX_ASYNC_MODE
	/* try to release some kernel resources */
	ftdi_async_complete(ftdi, 1);
#endif

	ret = usb_release_interface(ftdic.usb_dev, ftdic.interface);
	if (ret < 0) {
		fprintf(stderr, "unable to release interface (%s)\n",
			strerror(errno));
		return EXIT_FAILURE;
	}

	ret = usb_attach_kernel_driver_np(ftdic.usb_dev, ftdic.interface);
	if (ret < 0) {
		fprintf(stderr, "unable to reattach the driver (%s)\n",
			strerror(errno));
		return EXIT_FAILURE;
	}

	ftdi_usb_close(&ftdic);
	ftdi_deinit(&ftdic);

	return 0;
}
