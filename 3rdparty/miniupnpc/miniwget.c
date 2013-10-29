/* $Id: miniwget.c,v 1.60 2013/10/07 10:03:16 nanard Exp $ */
/* Project : miniupnp
 * Website : http://miniupnp.free.fr/
 * Author : Thomas Bernard
 * Copyright (c) 2005-2013 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#define MAXHOSTNAMELEN 64
#define MIN(x,y) (((x)<(y))?(x):(y))
#define snprintf _snprintf
#define socklen_t int
#ifndef strncasecmp
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#define strncasecmp _memicmp
#else /* defined(_MSC_VER) && (_MSC_VER >= 1400) */
#define strncasecmp memicmp
#endif /* defined(_MSC_VER) && (_MSC_VER >= 1400) */
#endif /* #ifndef strncasecmp */
#else /* #ifdef _WIN32 */
#include <unistd.h>
#include <sys/param.h>
#if defined(__amigaos__) && !defined(__amigaos4__)
#define socklen_t int
#else /* #if defined(__amigaos__) && !defined(__amigaos4__) */
#include <sys/select.h>
#endif /* #else defined(__amigaos__) && !defined(__amigaos4__) */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#define closesocket close
/* defining MINIUPNPC_IGNORE_EINTR enable the ignore of interruptions
 * during the connect() call */
#define MINIUPNPC_IGNORE_EINTR
#endif /* #else _WIN32 */
#if defined(__sun) || defined(sun)
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif

#include "miniupnpcstrings.h"
#include "miniwget.h"
#include "connecthostport.h"
#include "receivedata.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

/*
 * Read a HTTP response from a socket.
 * Process Content-Length and Transfer-encoding headers.
 * return a pointer to the content buffer, which length is saved
 * to the length parameter.
 */
void *
getHTTPResponse(int s, int * size)
{
	char buf[2048];
	int n;
	int endofheaders = 0;
	int chunked = 0;
	int content_length = -1;
	unsigned int chunksize = 0;
	unsigned int bytestocopy = 0;
	/* buffers : */
	char * header_buf;
	unsigned int header_buf_len = 2048;
	unsigned int header_buf_used = 0;
	char * content_buf;
	unsigned int content_buf_len = 2048;
	unsigned int content_buf_used = 0;
	char chunksize_buf[32];
	unsigned int chunksize_buf_index;

	header_buf = malloc(header_buf_len);
	content_buf = malloc(content_buf_len);
	chunksize_buf[0] = '\0';
	chunksize_buf_index = 0;

	while((n = receivedata(s, buf, 2048, 5000, NULL)) > 0)
	{
		if(endofheaders == 0)
		{
			int i;
			int linestart=0;
			int colon=0;
			int valuestart=0;
			if(header_buf_used + n > header_buf_len) {
				header_buf = realloc(header_buf, header_buf_used + n);
				header_buf_len = header_buf_used + n;
			}
			memcpy(header_buf + header_buf_used, buf, n);
			header_buf_used += n;
			/* search for CR LF CR LF (end of headers)
			 * recognize also LF LF */
			i = 0;
			while(i < ((int)header_buf_used-1) && (endofheaders == 0)) {
				if(header_buf[i] == '\r') {
					i++;
					if(header_buf[i] == '\n') {
						i++;
						if(i < (int)header_buf_used && header_buf[i] == '\r') {
							i++;
							if(i < (int)header_buf_used && header_buf[i] == '\n') {
								endofheaders = i+1;
							}
						}
					}
				} else if(header_buf[i] == '\n') {
					i++;
					if(header_buf[i] == '\n') {
						endofheaders = i+1;
					}
				}
				i++;
			}
			if(endofheaders == 0)
				continue;
			/* parse header lines */
			for(i = 0; i < endofheaders - 1; i++) {
				if(colon <= linestart && header_buf[i]==':')
				{
					colon = i;
					while(i < (endofheaders-1)
					      && (header_buf[i+1] == ' ' || header_buf[i+1] == '\t'))
						i++;
					valuestart = i + 1;
				}
				/* detecting end of line */
				else if(header_buf[i]=='\r' || header_buf[i]=='\n')
				{
					if(colon > linestart && valuestart > colon)
					{
#ifdef DEBUG
						printf("header='%.*s', value='%.*s'\n",
						       colon-linestart, header_buf+linestart,
						       i-valuestart, header_buf+valuestart);
#endif
						if(0==strncasecmp(header_buf+linestart, "content-length", colon-linestart))
						{
							content_length = atoi(header_buf+valuestart);
#ifdef DEBUG
							printf("Content-Length: %d\n", content_length);
#endif
						}
						else if(0==strncasecmp(header_buf+linestart, "transfer-encoding", colon-linestart)
						   && 0==strncasecmp(header_buf+valuestart, "chunked", 7))
						{
#ifdef DEBUG
							printf("chunked transfer-encoding!\n");
#endif
							chunked = 1;
						}
					}
					while((i < (int)header_buf_used) && (header_buf[i]=='\r' || header_buf[i] == '\n'))
						i++;
					linestart = i;
					colon = linestart;
					valuestart = 0;
				}
			}
			/* copy the remaining of the received data back to buf */
			n = header_buf_used - endofheaders;
			memcpy(buf, header_buf + endofheaders, n);
			/* if(headers) */
		}
		if(endofheaders)
		{
			/* content */
			if(chunked)
			{
				int i = 0;
				while(i < n)
				{
					if(chunksize == 0)
					{
						/* reading chunk size */
						if(chunksize_buf_index == 0) {
							/* skipping any leading CR LF */
							if(i<n && buf[i] == '\r') i++;
							if(i<n && buf[i] == '\n') i++;
						}
						while(i<n && isxdigit(buf[i])
						     && chunksize_buf_index < (sizeof(chunksize_buf)-1))
						{
							chunksize_buf[chunksize_buf_index++] = buf[i];
							chunksize_buf[chunksize_buf_index] = '\0';
							i++;
						}
						while(i<n && buf[i] != '\r' && buf[i] != '\n')
							i++; /* discarding chunk-extension */
						if(i<n && buf[i] == '\r') i++;
						if(i<n && buf[i] == '\n') {
							unsigned int j;
							for(j = 0; j < chunksize_buf_index; j++) {
							if(chunksize_buf[j] >= '0'
							   && chunksize_buf[j] <= '9')
								chunksize = (chunksize << 4) + (chunksize_buf[j] - '0');
							else
								chunksize = (chunksize << 4) + ((chunksize_buf[j] | 32) - 'a' + 10);
							}
							chunksize_buf[0] = '\0';
							chunksize_buf_index = 0;
							i++;
						} else {
							/* not finished to get chunksize */
							continue;
						}
#ifdef DEBUG
						printf("chunksize = %u (%x)\n", chunksize, chunksize);
#endif
						if(chunksize == 0)
						{
#ifdef DEBUG
							printf("end of HTTP content - %d %d\n", i, n);
							/*printf("'%.*s'\n", n-i, buf+i);*/
#endif
							goto end_of_stream;
						}
					}
					bytestocopy = ((int)chunksize < (n - i))?chunksize:(unsigned int)(n - i);
					if((content_buf_used + bytestocopy) > content_buf_len)
					{
						if(content_length >= (int)(content_buf_used + bytestocopy)) {
							content_buf_len = content_length;
						} else {
							content_buf_len = content_buf_used + bytestocopy;
						}
						content_buf = (char *)realloc((void *)content_buf,
						                              content_buf_len);
					}
					memcpy(content_buf + content_buf_used, buf + i, bytestocopy);
					content_buf_used += bytestocopy;
					i += bytestocopy;
					chunksize -= bytestocopy;
				}
			}
			else
			{
				/* not chunked */
				if(content_length > 0
				   && (int)(content_buf_used + n) > content_length) {
					/* skipping additional bytes */
					n = content_length - content_buf_used;
				}
				if(content_buf_used + n > content_buf_len)
				{
					if(content_length >= (int)(content_buf_used + n)) {
						content_buf_len = content_length;
					} else {
						content_buf_len = content_buf_used + n;
					}
					content_buf = (char *)realloc((void *)content_buf,
					                              content_buf_len);
				}
				memcpy(content_buf + content_buf_used, buf, n);
				content_buf_used += n;
			}
		}
		/* use the Content-Length header value if available */
		if(content_length > 0 && (int)content_buf_used >= content_length)
		{
#ifdef DEBUG
			printf("End of HTTP content\n");
#endif
			break;
		}
	}
end_of_stream:
	free(header_buf); header_buf = NULL;
	*size = content_buf_used;
	if(content_buf_used == 0)
	{
		free(content_buf);
		content_buf = NULL;
	}
	return content_buf;
}

/* miniwget3() :
 * do all the work.
 * Return NULL if something failed. */
static void *
miniwget3(const char * host,
          unsigned short port, const char * path,
          int * size, char * addr_str, int addr_str_len,
          const char * httpversion, unsigned int scope_id)
{
	char buf[2048];
    int s;
	int n;
	int len;
	int sent;
	void * content;

	*size = 0;
	s = connecthostport(host, port, scope_id);
	if(s < 0)
		return NULL;

	/* get address for caller ! */
	if(addr_str)
	{
		struct sockaddr_storage saddr;
		socklen_t saddrlen;

		saddrlen = sizeof(saddr);
		if(getsockname(s, (struct sockaddr *)&saddr, &saddrlen) < 0)
		{
			perror("getsockname");
		}
		else
		{
#if defined(__amigaos__) && !defined(__amigaos4__)
	/* using INT WINAPI WSAAddressToStringA(LPSOCKADDR, DWORD, LPWSAPROTOCOL_INFOA, LPSTR, LPDWORD);
     * But his function make a string with the port :  nn.nn.nn.nn:port */
/*		if(WSAAddressToStringA((SOCKADDR *)&saddr, sizeof(saddr),
                            NULL, addr_str, (DWORD *)&addr_str_len))
		{
		    printf("WSAAddressToStringA() failed : %d\n", WSAGetLastError());
		}*/
			/* the following code is only compatible with ip v4 addresses */
			strncpy(addr_str, inet_ntoa(((struct sockaddr_in *)&saddr)->sin_addr), addr_str_len);
#else
#if 0
			if(saddr.sa_family == AF_INET6) {
				inet_ntop(AF_INET6,
				          &(((struct sockaddr_in6 *)&saddr)->sin6_addr),
				          addr_str, addr_str_len);
			} else {
				inet_ntop(AF_INET,
				          &(((struct sockaddr_in *)&saddr)->sin_addr),
				          addr_str, addr_str_len);
			}
#endif
			/* getnameinfo return ip v6 address with the scope identifier
			 * such as : 2a01:e35:8b2b:7330::%4281128194 */
			n = getnameinfo((const struct sockaddr *)&saddr, saddrlen,
			                addr_str, addr_str_len,
			                NULL, 0,
			                NI_NUMERICHOST | NI_NUMERICSERV);
			if(n != 0) {
#ifdef _WIN32
				fprintf(stderr, "getnameinfo() failed : %d\n", n);
#else
				fprintf(stderr, "getnameinfo() failed : %s\n", gai_strerror(n));
#endif
			}
#endif
		}
#ifdef DEBUG
		printf("address miniwget : %s\n", addr_str);
#endif
	}

	len = snprintf(buf, sizeof(buf),
                 "GET %s HTTP/%s\r\n"
			     "Host: %s:%d\r\n"
				 "Connection: Close\r\n"
				 "User-Agent: " OS_STRING ", UPnP/1.0, MiniUPnPc/" MINIUPNPC_VERSION_STRING "\r\n"

				 "\r\n",
			   path, httpversion, host, port);
	sent = 0;
	/* sending the HTTP request */
	while(sent < len)
	{
		n = send(s, buf+sent, len-sent, 0);
		if(n < 0)
		{
			perror("send");
			closesocket(s);
			return NULL;
		}
		else
		{
			sent += n;
		}
	}
	content = getHTTPResponse(s, size);
	closesocket(s);
	return content;
}

/* miniwget2() :
 * Call miniwget3(); retry with HTTP/1.1 if 1.0 fails. */
static void *
miniwget2(const char * host,
		  unsigned short port, const char * path,
		  int * size, char * addr_str, int addr_str_len,
          unsigned int scope_id)
{
	char * respbuffer;

#if 1
	respbuffer = miniwget3(host, port, path, size,
	                       addr_str, addr_str_len, "1.1", scope_id);
#else
	respbuffer = miniwget3(host, port, path, size,
	                       addr_str, addr_str_len, "1.0", scope_id);
	if (*size == 0)
	{
#ifdef DEBUG
		printf("Retrying with HTTP/1.1\n");
#endif
		free(respbuffer);
		respbuffer = miniwget3(host, port, path, size,
		                       addr_str, addr_str_len, "1.1", scope_id);
	}
#endif
	return respbuffer;
}




/* parseURL()
 * arguments :
 *   url :		source string not modified
 *   hostname :	hostname destination string (size of MAXHOSTNAMELEN+1)
 *   port :		port (destination)
 *   path :		pointer to the path part of the URL
 *
 * Return values :
 *    0 - Failure
 *    1 - Success         */
int
parseURL(const char * url,
         char * hostname, unsigned short * port,
         char * * path, unsigned int * scope_id)
{
	char * p1, *p2, *p3;
	if(!url)
		return 0;
	p1 = strstr(url, "://");
	if(!p1)
		return 0;
	p1 += 3;
	if(  (url[0]!='h') || (url[1]!='t')
	   ||(url[2]!='t') || (url[3]!='p'))
		return 0;
	memset(hostname, 0, MAXHOSTNAMELEN + 1);
	if(*p1 == '[')
	{
		/* IP v6 : http://[2a00:1450:8002::6a]/path/abc */
		char * scope;
		scope = strchr(p1, '%');
		p2 = strchr(p1, ']');
		if(p2 && scope && scope < p2 && scope_id) {
			/* parse scope */
#ifdef IF_NAMESIZE
			char tmp[IF_NAMESIZE];
			int l;
			scope++;
			/* "%25" is just '%' in URL encoding */
			if(scope[0] == '2' && scope[1] == '5')
				scope += 2;	/* skip "25" */
			l = p2 - scope;
			if(l >= IF_NAMESIZE)
				l = IF_NAMESIZE - 1;
			memcpy(tmp, scope, l);
			tmp[l] = '\0';
			*scope_id = if_nametoindex(tmp);
			if(*scope_id == 0) {
				*scope_id = (unsigned int)strtoul(tmp, NULL, 10);
			}
#else
			/* under windows, scope is numerical */
			char tmp[8];
			int l;
			scope++;
			/* "%25" is just '%' in URL encoding */
			if(scope[0] == '2' && scope[1] == '5')
				scope += 2;	/* skip "25" */
			l = p2 - scope;
			if(l >= sizeof(tmp))
				l = sizeof(tmp) - 1;
			memcpy(tmp, scope, l);
			tmp[l] = '\0';
			*scope_id = (unsigned int)strtoul(tmp, NULL, 10);
#endif
		}
		p3 = strchr(p1, '/');
		if(p2 && p3)
		{
			p2++;
			strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p2-p1)));
			if(*p2 == ':')
			{
				*port = 0;
				p2++;
				while( (*p2 >= '0') && (*p2 <= '9'))
				{
					*port *= 10;
					*port += (unsigned short)(*p2 - '0');
					p2++;
				}
			}
			else
			{
				*port = 80;
			}
			*path = p3;
			return 1;
		}
	}
	p2 = strchr(p1, ':');
	p3 = strchr(p1, '/');
	if(!p3)
		return 0;
	if(!p2 || (p2>p3))
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p3-p1)));
		*port = 80;
	}
	else
	{
		strncpy(hostname, p1, MIN(MAXHOSTNAMELEN, (int)(p2-p1)));
		*port = 0;
		p2++;
		while( (*p2 >= '0') && (*p2 <= '9'))
		{
			*port *= 10;
			*port += (unsigned short)(*p2 - '0');
			p2++;
		}
	}
	*path = p3;
	return 1;
}

void *
miniwget(const char * url, int * size, unsigned int scope_id)
{
	unsigned short port;
	char * path;
	/* protocol://host:port/chemin */
	char hostname[MAXHOSTNAMELEN+1];
	*size = 0;
	if(!parseURL(url, hostname, &port, &path, &scope_id))
		return NULL;
#ifdef DEBUG
	printf("parsed url : hostname='%s' port=%hu path='%s' scope_id=%u\n",
	       hostname, port, path, scope_id);
#endif
	return miniwget2(hostname, port, path, size, 0, 0, scope_id);
}

void *
miniwget_getaddr(const char * url, int * size,
                 char * addr, int addrlen, unsigned int scope_id)
{
	unsigned short port;
	char * path;
	/* protocol://host:port/path */
	char hostname[MAXHOSTNAMELEN+1];
	*size = 0;
	if(addr)
		addr[0] = '\0';
	if(!parseURL(url, hostname, &port, &path, &scope_id))
		return NULL;
#ifdef DEBUG
	printf("parsed url : hostname='%s' port=%hu path='%s' scope_id=%u\n",
	       hostname, port, path, scope_id);
#endif
	return miniwget2(hostname, port, path, size, addr, addrlen, scope_id);
}

