#include "protocol.h"
#include <unistd.h>
#include <stdlib.h>

int proto_send_packet(int fd, CHLA_PACKET_HEADER *hdr, void *payload) {
	size_t headerSize = sizeof(CHLA_PACKET_HEADER);

	if(write(fd, (void*) hdr, headerSize) < 0) {
		return -1;
	}

	int payloadSize = hdr->payload_length;
	payloadSize = htonl(payloadSize);
	if(payload != NULL && payloadSize != 0) {
		if(write(fd, payload, payloadSize) < 0) {
			return -1;
		}
	}

	return 0;
}

int proto_recv_packet(int fd, CHLA_PACKET_HEADER *hdr, void **payload) {
	size_t headerSize = sizeof(CHLA_PACKET_HEADER);

	if(read(fd, (void *) hdr, headerSize) < 0) {
		return -1;
	}
	int payloadSize = ntohl(hdr->payload_length);


	if(payloadSize != 0 && payload != NULL) {
		char *buffer = malloc(sizeof(char) * payloadSize);
		if(buffer == NULL) {
			return -1;
		}

		int bytesRead = read(fd, buffer, payloadSize);

		if(bytesRead == 0 || bytesRead < 0) {
			return -1;
		}

		if(bytesRead < payloadSize) {
			free(buffer);
			return -1;
		}

		*payload = buffer;

	}

	return 0;
}