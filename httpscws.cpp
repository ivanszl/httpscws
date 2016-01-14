/*
 *  HTTPSCWS - HTTPSCWS is an Chinese Word Segmentation System Based on the HTTP protocol.
 *
 *
 *  Copyright 2016 Ivan Lam.  All rights reserved.
 *
 *  Use and distribution licensed under the BSD license. 
 *
 *  Authors:
 *      Ivan Lam <ivan.lin.1985@gmail.com> http://blog.linsongzheng.com
 */

extern "C" {
#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <wait.h>

#include <signal.h>
#include <err.h>
#include <event.h>
#include <evhttp.h>

#define VERSION "1.0.0"
#define SCWS_PREFIX "/usr/local/"
}
#include "scws.h"

scws_t g_scws;

/* 去除字符串两端的空格 */
void trim( char *str )
{
        char *copied, *tail = NULL;

        if ( str == NULL )
                return;

        for( copied = str; *str; str++ )
        {
                if ( *str != ' ' && *str != '\t' )
                {
                        *copied++ = *str;
                         tail = copied;
                }
                else
                {
                         if ( tail )
                                 *copied++ = *str;
                }
        }

        if ( tail )
             *tail = 0;
        else
             *copied = 0;

        return;
}

char *urldecode(char *in)
{
    char  *out;
    char   temp[32];
    char  *p, *q;
    int             value;

    out = (char *) malloc(strlen(in) + 1);

    p = in, q = out;
    while (*p != 0) {
        if (*p == '+') {
            *q++ = ' ';
        } else if (*p == '%') {
            temp[0] = *(p + 1);
            temp[1] = *(p + 2);
            temp[2] = 0;
            sscanf(temp, "%x", &value);

            *q++ = 0xff & value;

            p += 2;
        } else {
            *q++ = *p;
        }

        p++;
    }
    *q = '\0';

    return (char *) out;
}

void signal_handler(int sig)
{
	switch (sig) {
		case SIGTERM:
		case SIGHUP:
		case SIGQUIT:
		case SIGINT:
			event_loopbreak();
			break;
	}
}

static void show_help(void)
{
	char *b = "HTTPSCWS v" VERSION " written by Ivan Lam (http://blog.linsongzheng.com)\n"
		  "\n"
		   "-l <ip_addr>  interface to listen on, default is 0.0.0.0\n"
		   "-p <num>      TCP port number to listen on (default: 1985)\n"
		   "-x <path>     SCWS dictionary and rule path (example: /etc/scws/)\n"
		   "-i <filename> create the pid file path (example: /var/run/httpscws.pid)\n"
		   "-t <second>   timeout for an http request (default: 120)\n"		   
		   "-d            run as a daemon\n"
		   "-h            print this help and exit\n"	   
		   "\n";
	fprintf(stderr, b, strlen(b));
}

/* 处理模块 */
void httpscws_handler(struct evhttp_request *req, void *arg)
{
	struct evbuffer *buf;
	buf = evbuffer_new();

	scws_res_t res, cur;

	/* 分析URL参数 */
	struct evkeyvalq httpscws_http_query;
	evhttp_parse_query(evhttp_request_uri(req), &httpscws_http_query);

	/* 接收POST表单信息 */
	const char *httpscws_input_postbuffer = (const char*) EVBUFFER_DATA(req->input_buffer);

	/* 接收GET表单参数 */
	const char *httpscws_input_words = evhttp_find_header(&httpscws_http_query, "w"); 

	const char *httpscws_output_tmp = NULL;

	if (httpscws_input_postbuffer != NULL) {
		int buffer_len = EVBUFFER_LENGTH( req->input_buffer ) + 1;
		char *httpscws_input_postbuffer_tmp = (char *) malloc( buffer_len );
		memset( httpscws_input_postbuffer_tmp, '\0', buffer_len );
		strncpy( httpscws_input_postbuffer_tmp, httpscws_input_postbuffer, buffer_len - 1 );
		char *decode_uri = urldecode( httpscws_input_postbuffer_tmp );
		free( httpscws_input_postbuffer_tmp );
		scws_send_text(g_scws, decode_uri, strlen(decode_uri));
		while ( res = cur = scws_get_result(g_scws))
		{
			while ( cur != NULL)
			{
				evbuffer_add_printf(buf, "%.*s ", cur->len, decode_uri + cur->off);
				// printf("WORD: %.*s/%s (IDF = %4.2f)\n", cur->len, decode_uri + cur->off, cur->attr, cur->idf);
				cur = cur->next;
			}
			scws_free_result(res);
		}
		free( decode_uri );
	}
	else if (httpscws_input_words != NULL)
	{
		char *httpscws_input_words_tmp = strdup( httpscws_input_words );
		char *decode_uri = urldecode( httpscws_input_words_tmp );
		free(httpscws_input_words_tmp );
		scws_send_text(g_scws, decode_uri, strlen(decode_uri));
		while ( res = cur = scws_get_result(g_scws))
		{
			while ( cur != NULL)
			{
				evbuffer_add_printf(buf, "%.*s ", cur->len, decode_uri + cur->off);
				// printf("WORD: %.*s/%s (IDF = %4.2f)\n", cur->len, decode_uri + cur->off, cur->attr, cur->idf);
				cur = cur->next;
			}
			scws_free_result(res);
		}
		free( decode_uri );
	} else {
		evbuffer_add_printf(buf, "");
	}

	/* 输出内容给客户端 */
	evhttp_add_header(req->output_headers, "Server", "HTTPSCWS/1.0.0");
	evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=utf8");
	evhttp_add_header(req->output_headers, "Connection", "close");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	evhttp_clear_headers( &httpscws_http_query );
	evbuffer_free( buf );
}

int main(int argc, char **argv)
{
	/* 自定义信号处理函数 */
	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);

	int c;
	/* 默认参数设置 */
	char *httpscws_settings_listen   = "0.0.0.0";
	int   httpscws_settings_port     = 1985;
	char *httpscws_settings_datapath = NULL; /*中文词典数据库路径 */
	char *httpscws_settings_pid = NULL;
	bool  httpscws_settings_daemon   = false;
	int   httpscws_settings_timeout  = 120; /* 单位：秒 */

    /* process arguments */
    while ((c = getopt(argc, argv, "l:p:x:i:t:dh")) != -1) {
        switch (c) {
        case 'l':
            httpscws_settings_listen = strdup(optarg);
            break;
        case 'p':
            httpscws_settings_port = atoi(optarg);
            break;
        case 'x':
            httpscws_settings_datapath = strdup(optarg); /* 词库文件存储路径 */
            break;
        case 'i':
        	httpscws_settings_pid = strdup(optarg);
        	break;
        case 't':
            httpscws_settings_timeout = atoi(optarg);
            break;			
        case 'd':
            httpscws_settings_daemon = true;
            break;
		case 'h':			
        default:
            show_help();
            return 1;
        }
    }
	
	/* 判断是否加了必填参数 -x */
	if (httpscws_settings_datapath == NULL || httpscws_settings_pid == NULL) {
		show_help();
		fprintf(stderr, "Attention: Please use the indispensable argument: -x <path> -i <filename>\n\n");		
		exit(1);
	}
	/* 初始化分词组件 */
	if (!(g_scws = scws_new())) {
		fprintf(stderr, "ERROR: Cann't init the scws!\n\n");
		exit(1);
	}
	scws_set_charset(g_scws, "utf8");
	fprintf(stderr, "Loading SCWS dictionary 'dict.utf8.xdb' into memory, please waitting ......\n");

	char   *tmp_path = (char *)malloc(1024);
	memset (tmp_path, '\0', 1024);
	sprintf(tmp_path, "%s/dict.utf8.xdb", httpscws_settings_datapath);

	scws_set_dict(g_scws, tmp_path, SCWS_XDICT_XDB);
	memset (tmp_path, '\0', 1024);
	sprintf(tmp_path, "%s/rules.utf8.ini", httpscws_settings_datapath);
	scws_set_rule(g_scws, tmp_path);
	scws_set_ignore(g_scws, 1);

	free( tmp_path );
	printf("HTTPSCWS Server running on %s:%d\n", httpscws_settings_listen, httpscws_settings_port);

	/* 如果加了-d参数，以守护进程运行 */
	if (httpscws_settings_daemon == true){
        pid_t pid;

        /* Fork off the parent process */       
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }
	}
	char      pid[256];
	memset (pid, '\0', 256);
	sprintf(pid, "%ld", getpid());
	int fd = open((const char *) httpscws_settings_pid, O_RDWR|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		fprintf(stderr, "Can't create the pid file\n");
		exit(EXIT_FAILURE);
	}
	write(fd, pid, strlen(pid));
	close(fd);
	
	/* 请求处理部分 */
	struct evhttp *httpd;

	event_init();
	httpd = evhttp_start(httpscws_settings_listen, httpscws_settings_port);
	evhttp_set_timeout(httpd, httpscws_settings_timeout);

	/* Set a callback for all other requests. */
	evhttp_set_gencb(httpd, httpscws_handler, NULL);

	event_dispatch();

	/* Not reached in this code as it is now. */
	evhttp_free(httpd);
	/* 释放内存 */
	scws_free(g_scws);
	printf("HTTPSCWS stop\n");

    return 0;
}


