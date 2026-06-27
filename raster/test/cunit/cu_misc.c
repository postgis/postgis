/*
 * PostGIS Raster - Raster Types for PostGIS
 * http://trac.osgeo.org/postgis/wiki/WKTRaster
 *
 * Copyright (C) 2013 Regents of the University of California
 *   <bkpark@ucdavis.edu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "CUnit/Basic.h"
#include "cu_tester.h"

#ifndef _WIN32
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

static void test_rgb_to_hsv(void) {
	double rgb[3] = {0, 0, 0};
	double hsv[3] = {0, 0, 0};

	rt_util_rgb_to_hsv(rgb, hsv);
	CU_ASSERT_DOUBLE_EQUAL(hsv[0], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[1], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[2], 0, DBL_EPSILON);

	rgb[0] = 0;
	rgb[1] = 0;
	rgb[2] = 1;
	rt_util_rgb_to_hsv(rgb, hsv);
	CU_ASSERT_DOUBLE_EQUAL(hsv[0], 2/3., DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[1], 1, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[2], 1, DBL_EPSILON);

	rgb[0] = 0;
	rgb[1] = 0.25;
	rgb[2] = 0.5;
	rt_util_rgb_to_hsv(rgb, hsv);
	CU_ASSERT_DOUBLE_EQUAL(hsv[0], 7/12., DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[1], 1, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[2], 0.5, DBL_EPSILON);

	rgb[0] = 0.5;
	rgb[1] = 1;
	rgb[2] = 0.5;
	rt_util_rgb_to_hsv(rgb, hsv);
	CU_ASSERT_DOUBLE_EQUAL(hsv[0], 1/3., DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[1], 0.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[2], 1, DBL_EPSILON);

	rgb[0] = 0.2;
	rgb[1] = 0.4;
	rgb[2] = 0.4;
	rt_util_rgb_to_hsv(rgb, hsv);
	CU_ASSERT_DOUBLE_EQUAL(hsv[0], 0.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[1], 0.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(hsv[2], 0.4, DBL_EPSILON);
}

static void test_hsv_to_rgb(void) {
	double hsv[3] = {0, 0, 0};
	double rgb[3] = {0, 0, 0};

	rt_util_hsv_to_rgb(hsv, rgb);
	CU_ASSERT_DOUBLE_EQUAL(rgb[0], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[1], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[2], 0, DBL_EPSILON);

	hsv[0] = 2/3.;
	hsv[1] = 1;
	hsv[2] = 1;
	rt_util_hsv_to_rgb(hsv, rgb);
	CU_ASSERT_DOUBLE_EQUAL(rgb[0], 0., DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[1], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[2], 1, DBL_EPSILON);

	hsv[0] = 7/12.;
	hsv[1] = 1;
	hsv[2] = 0.5;
	rt_util_hsv_to_rgb(hsv, rgb);
	CU_ASSERT_DOUBLE_EQUAL(rgb[0], 0, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[1], 0.25, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[2], 0.5, DBL_EPSILON);

	hsv[0] = 1/3.;
	hsv[1] = 0.5;
	hsv[2] = 1;
	rt_util_hsv_to_rgb(hsv, rgb);
	CU_ASSERT_DOUBLE_EQUAL(rgb[0], 0.5, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[1], 1, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[2], 0.5, DBL_EPSILON);

	hsv[0] = 0.5;
	hsv[1] = 0.5;
	hsv[2] = 0.4;
	rt_util_hsv_to_rgb(hsv, rgb);
	CU_ASSERT_DOUBLE_EQUAL(rgb[0], 0.2, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[1], 0.4, DBL_EPSILON);
	CU_ASSERT_DOUBLE_EQUAL(rgb[2], 0.4, DBL_EPSILON);
}

#ifndef _WIN32
static int
create_local_http_socket(uint16_t *port)
{
	int fd;
	int reuse = 1;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
		goto error;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		goto error;

	if (listen(fd, 8) < 0)
		goto error;

	if (getsockname(fd, (struct sockaddr *)&addr, &addrlen) < 0)
		goto error;

	*port = ntohs(addr.sin_port);
	return fd;

error:
	close(fd);
	return -1;
}

static int
send_all(int fd, const void *buf, size_t len)
{
	const char *ptr = (const char *)buf;
	while (len > 0)
	{
		ssize_t written = send(fd, ptr, len, 0);
		if (written < 0)
		{
			if (errno == EINTR)
				continue;
			return -1;
		}
		ptr += written;
		len -= written;
	}
	return 0;
}

static void
serve_local_file(int server_fd, const char *path)
{
	int i;
	struct stat st;

	signal(SIGPIPE, SIG_IGN);
	alarm(20);

	if (stat(path, &st) != 0)
		_exit(EXIT_FAILURE);

	for (i = 0; i < 16; i++)
	{
		char request[2048];
		char header[512];
		int client_fd;
		ssize_t request_len;
		int is_head = 0;
		off_t start = 0;
		off_t end = st.st_size - 1;
		int partial = 0;
		const char *range;
		int file_fd;

		client_fd = accept(server_fd, NULL, NULL);
		if (client_fd < 0)
		{
			if (errno == EINTR)
				continue;
			_exit(EXIT_FAILURE);
		}

		request_len = recv(client_fd, request, sizeof(request) - 1, 0);
		if (request_len <= 0)
		{
			close(client_fd);
			continue;
		}
		request[request_len] = '\0';

		is_head = strncmp(request, "HEAD ", 5) == 0;
		range = strstr(request, "\r\nRange: bytes=");
		if (range)
		{
			unsigned long long range_start;
			unsigned long long range_end;
			const char *range_spec = range + strlen("\r\nRange: bytes=");
			int parsed = sscanf(range_spec, "%llu-%llu", &range_start, &range_end);
			if (parsed >= 1)
			{
				start = (off_t)range_start;
				if (parsed >= 2)
					end = (off_t)range_end;
				if (start < 0)
					start = 0;
				if (end >= st.st_size)
					end = st.st_size - 1;
				if (start <= end)
					partial = 1;
			}
		}

		if (partial)
		{
			snprintf(header,
				 sizeof(header),
				 "HTTP/1.1 206 Partial Content\r\n"
				 "Content-Type: image/tiff\r\n"
				 "Content-Length: %lld\r\n"
				 "Content-Range: bytes %lld-%lld/%lld\r\n"
				 "Accept-Ranges: bytes\r\n"
				 "Connection: close\r\n\r\n",
				 (long long)(end - start + 1),
				 (long long)start,
				 (long long)end,
				 (long long)st.st_size);
		}
		else
		{
			snprintf(header,
				 sizeof(header),
				 "HTTP/1.1 200 OK\r\n"
				 "Content-Type: image/tiff\r\n"
				 "Content-Length: %lld\r\n"
				 "Accept-Ranges: bytes\r\n"
				 "Connection: close\r\n\r\n",
				 (long long)st.st_size);
		}

		if (send_all(client_fd, header, strlen(header)) != 0 || is_head)
		{
			close(client_fd);
			continue;
		}

		file_fd = open(path, O_RDONLY);
		if (file_fd >= 0)
		{
			char buf[4096];
			off_t remaining = end - start + 1;
			if (lseek(file_fd, start, SEEK_SET) >= 0)
			{
				while (remaining > 0)
				{
					ssize_t nread =
					    read(file_fd,
						 buf,
						 remaining < (off_t)sizeof(buf) ? (size_t)remaining : sizeof(buf));
					if (nread <= 0)
						break;
					if (send_all(client_fd, buf, (size_t)nread) != 0)
						break;
					remaining -= nread;
				}
			}
			close(file_fd);
		}
		close(client_fd);
	}

	_exit(EXIT_SUCCESS);
}
#endif

static void test_util_gdal_open(void) {
	extern char *gdal_enabled_drivers;

	GDALDatasetH ds;

	char *disable_all = GDAL_DISABLE_ALL;
	char *enabled = "GTiff JPEG PNG";
#ifndef _WIN32
	char *enabled_vsi = "GTiff JPEG PNG VSICURL";
	int server_fd;
	uint16_t server_port = 0;
	pid_t server_pid = -1;
	char raster_path[1024];
	char vsi_url[128];
#endif

	rt_util_gdal_register_all(1);

	/* all drivers disabled */
	gdal_enabled_drivers = disable_all;
	ds = rt_util_gdal_open("/tmp/foo", GA_ReadOnly, 0);
	CU_ASSERT(ds == NULL);

	/* can't test VSICURL if HTTP driver not found */
	if (!rt_util_gdal_driver_registered("HTTP"))
	{
		gdal_enabled_drivers = NULL;
		return;
	}

	/* enabled drivers, no VSICURL */
	gdal_enabled_drivers = enabled;
	ds = rt_util_gdal_open("/vsicurl/http://127.0.0.1/postgis-test.tif", GA_ReadOnly, 0);
	CU_ASSERT(ds == NULL);

#ifndef _WIN32
	snprintf(raster_path, sizeof(raster_path), "%s/raster/test/regress/loader/testraster.tif", POSTGIS_TOP_SRC_DIR);
	server_fd = create_local_http_socket(&server_port);
	CU_ASSERT(server_fd >= 0);
	if (server_fd < 0)
	{
		gdal_enabled_drivers = NULL;
		return;
	}

	server_pid = fork();
	CU_ASSERT(server_pid >= 0);
	if (server_pid == 0)
		serve_local_file(server_fd, raster_path);

	close(server_fd);
	if (server_pid < 0)
	{
		gdal_enabled_drivers = NULL;
		return;
	}

	snprintf(vsi_url, sizeof(vsi_url), "/vsicurl/http://127.0.0.1:%u/postgis-test.tif", server_port);
	CPLSetConfigOption("GDAL_HTTP_TIMEOUT", "5");
	CPLSetConfigOption("GDAL_HTTP_CONNECTTIMEOUT", "5");

	/* enabled drivers with VSICURL */
	gdal_enabled_drivers = enabled_vsi;
	ds = rt_util_gdal_open(vsi_url, GA_ReadOnly, 0);
	CU_ASSERT(ds != NULL);
	if (ds != NULL)
		GDALClose(ds);

	kill(server_pid, SIGTERM);
	waitpid(server_pid, NULL, 0);
	CPLSetConfigOption("GDAL_HTTP_TIMEOUT", NULL);
	CPLSetConfigOption("GDAL_HTTP_CONNECTTIMEOUT", NULL);
#endif

	gdal_enabled_drivers = NULL;
}

/* register tests */
void misc_suite_setup(void);
void misc_suite_setup(void)
{
	CU_pSuite suite = CU_add_suite("misc", NULL, NULL);
	PG_ADD_TEST(suite, test_rgb_to_hsv);
	PG_ADD_TEST(suite, test_hsv_to_rgb);
	PG_ADD_TEST(suite, test_util_gdal_open);
}
