#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <caenrfid.h>

static char nibble2hex(char c)
{
        switch (c) {
        case 0 ... 9:
                return '0' + c;

        case 0xa ... 0xf:
                return 'a' + (c - 10);
        }

        printf("got invalid data!");
        return '\0';
}

char *bin2hex(uint8_t *data, size_t len)
{
        char *str;
        int i;

        str = malloc(len * 2 + 1);
        if (!str)
                return NULL;

        for (i = 0; i < len; i++) {
                str[i * 2] = nibble2hex(data[i] >> 4);
                str[i * 2 + 1] = nibble2hex(data[i] & 0x0f);
        }
        str[i * 2] = '\0';

        return str;
}

int main(int argc, char *argv[])
{
	int i;
	struct caenrfid_handle handle;
        char string[] = "Source_0";
	struct caenrfid_tag *tag;
        size_t size;
        char *str;
        int ret;

	if (argc < 2) {
		fprintf(stderr, "usage: %s: <serial_port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

        /* Start a new connection with the CAENRFIDD server */
        ret = caenrfid_open(CAENRFID_PORT_RS232, argv[1], &handle);
        if (ret < 0) {
                fprintf(stderr, "cannot init lib (err=%d)\n", ret);
                exit(EXIT_FAILURE);
        }

	/* Do the inventory */
        ret = caenrfid_inventory(&handle, string, &tag, &size);
        if (ret < 0) {
                fprintf(stderr, "cannot get data (err=%d)\n", ret);
                exit(EXIT_FAILURE);
        }

	/* Report results */
        for (i = 0; i < size; i++) {
                str = bin2hex(tag[i].id, tag[i].len);
                if (!str) {
                        fprintf(stderr, "cannot allocate memory!\n");
                        exit(EXIT_FAILURE);
                }

                printf("%.*s %.*s %.*s %d\n",
                        tag[i].len * 2, str,
                        CAENRFID_SOURCE_NAME_LEN, tag[i].source,
                        CAENRFID_READPOINT_NAME_LEN, tag[i].readpoint,
			tag[i].type);

                free(str);
        }

	/* Free inventory data */
	free(tag);

        /* Close the connection */
        ret = caenrfid_close(&handle);
        if (ret < 0)
                fprintf(stderr, "improper caenrfidlib closure!\n");

	return 0;
}
