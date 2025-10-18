// custom lwip configuration

#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

#include_next "lwipopts.h"

#ifndef LWIP_HTTPD_CGI
#define LWIP_HTTPD_CGI 1
#endif

#ifndef LWIP_HTTPD_SSI
#define LWIP_HTTPD_SSI 1
#endif

#ifndef LWIP_HTTPD_CUSTOM_FILES
#define LWIP_HTTPD_CUSTOM_FILES 1
#endif

#ifndef LWIP_HTTPD_DYNAMIC_HEADERS
#define LWIP_HTTPD_DYNAMIC_HEADERS 1
#endif

#endif