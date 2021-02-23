/*
 * Copyright (C) 2016 - 2018 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>

#include "lwip/sockets.h"
#include "netif/xadapter.h"
#include "lwipopts.h"
#include "xil_printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "xil_io.h"
#include "xparameters.h"

#define THREAD_STACKSIZE 1024
const char READ_REQUEST[] = "readRequest:";
const char WRITE_REQUEST[] = "writeRequest:";
const char WRITE_REPLY[] = "Write OK\r\n";

u16_t echo_port = 7;

void print_echo_app_header()
{
    xil_printf("%20s %6d %s\r\n", "echo server",
                        echo_port,
                        "$ telnet <board_ip> 7");

}

/* thread spawned for each connection */
void process_echo_request(void *p)
{
	int sd = (int)p;
	int RECV_BUF_SIZE = 2048;
	char recv_buf[RECV_BUF_SIZE];
	int n, nwrote;

	while (1) {
		/* read a max of RECV_BUF_SIZE bytes from socket */
		if ((n = read(sd, recv_buf, RECV_BUF_SIZE)) < 0) {
			xil_printf("%s: error reading from socket %d, closing socket\r\n", __FUNCTION__, sd);
			break;
		}

		/* break if the recved message = "quit" */
		if (!strncmp(recv_buf, "quit", 4))
			break;

		/* break if client closed connection */
		if (n <= 0)
			break;

		printf("Received String: %s\r\n",recv_buf);
		printf("recv_buf String Length: %d, read n = %d\r\n", strlen(recv_buf), n);

		/* echo all the content in recv_buf */
//		if ((nwrote = write(sd, recv_buf, n)) < 0) {
//			xil_printf("%s: ERROR responding to client echo request. received = %d, written = %d\r\n",
//					__FUNCTION__, n, nwrote);
//			xil_printf("Closing socket %d\r\n", sd);
//			break;
//		}

		/* parse received command
		 * is read request: read the specified address, send back value
		 * is write request: write the specified address, reply OK
		 * Address Max: 0x7FFFFFFF
		 */
		if (strncmp(recv_buf, READ_REQUEST, strlen(READ_REQUEST)) == 0)
		{

			u32 addr = strtoul(recv_buf+strlen(READ_REQUEST), '\0', 16) + XPAR_JESD204_0_BASEADDR;
			printf("Received Read Request, required address: 0x%lx\r\n",addr);
			printf("Received Read Request, required address: %ld\r\n",addr);
			u32 val = Xil_In32(addr);
			printf("Read Value: %lx\r\n",val);
			printf("Size of Int: %d\r\n",sizeof(int));
			itoa(val, recv_buf, 16);
			strcat(recv_buf, "\r\n");
			printf("strlen recv_buf : %d\r\n",strlen(recv_buf));
			printf("send back recv_buf : %s\r\n", recv_buf);

			if ((nwrote = write(sd, recv_buf, strlen(recv_buf))) < 0) {
				xil_printf("%s: ERROR responding to client echo request. written = %d\r\n", __FUNCTION__, nwrote);
				xil_printf("Closing socket %d\r\n", sd);
				break;
			}
		}
		else if (strncmp(recv_buf, WRITE_REQUEST, strlen(WRITE_REQUEST)) == 0)
		{
			char *ptr;
			char *subStr[3];
			ptr = strtok(recv_buf, ":");
			int subStrIdx = 0;
			while (ptr != NULL)
			{
				printf("ptr=%s\r\n", ptr);
				subStr[subStrIdx] = ptr;
				subStrIdx++;
				ptr = strtok(NULL, ":");
			}
			if (subStrIdx != 3)
			{
				xil_printf("%s: Warning subStrIdx = %d\r\n", __FUNCTION__, subStrIdx);
				continue;
			}
			u32 addr = strtoul(subStr[1], '\0', 16) + XPAR_JESD204_0_BASEADDR; // strtol return -1 if input number>0x7FFFFFFF
			u32 val = strtoul(subStr[2], '\0', 16);
			printf("Received Write Request, required address: 0x%lx\r\n",addr);
			printf("Received Write Request, required value  : 0x%lx\r\n",val);
			Xil_Out32(addr, val);

			if ((nwrote = write(sd, WRITE_REPLY, strlen(WRITE_REPLY))) < 0) {
				xil_printf("%s: ERROR fail to send WRITE_REPLY. written = %d\r\n", __FUNCTION__, nwrote);
				xil_printf("Closing socket %d\r\n", sd);
				break;
			}
		}
		else
		{
			printf("Warning: Invalid command received. Continue\r\n");
			continue;
		}



	}

	/* close connection */
	close(sd);
	vTaskDelete(NULL);
}

void echo_application_thread()
{
	int sock, new_sd;
	int size;
#if LWIP_IPV6==0
	struct sockaddr_in address, remote;

	memset(&address, 0, sizeof(address));

	if ((sock = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return;

	address.sin_family = AF_INET;
	address.sin_port = htons(echo_port);
	address.sin_addr.s_addr = INADDR_ANY;
#else
	struct sockaddr_in6 address, remote;

	memset(&address, 0, sizeof(address));

	address.sin6_len = sizeof(address);
	address.sin6_family = AF_INET6;
	address.sin6_port = htons(echo_port);

	memset(&(address.sin6_addr), 0, sizeof(address.sin6_addr));

	if ((sock = lwip_socket(AF_INET6, SOCK_STREAM, 0)) < 0)
		return;
#endif

	if (lwip_bind(sock, (struct sockaddr *)&address, sizeof (address)) < 0)
		return;

	lwip_listen(sock, 0);

	size = sizeof(remote);

	while (1) {
		if ((new_sd = lwip_accept(sock, (struct sockaddr *)&remote, (socklen_t *)&size)) > 0) {
			sys_thread_new("echos", process_echo_request,
				(void*)new_sd,
				THREAD_STACKSIZE,
				DEFAULT_THREAD_PRIO);
		}
	}
}
