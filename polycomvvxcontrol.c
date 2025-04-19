#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <csv.h>
#include <expat.h>
#ifdef STATIC
#define CURL_STATICLIB
#endif
#include <curl/curl.h>
#include "polycomvvx.h"

#define PROGRAM_VERSION "1.6"

#define CSV_BUFFER_SIZE 1024

//#ifdef _WIN32
//#define strcasecmp stricmp
//#endif

struct csv_callback_data {
  int firstrowdone;
  size_t column;
  int cmd;
  const char* fieldname_host;
  const char* username;
  const char* password;
  const char* fieldname_ext;
  const char* fieldname_pin;
  int timeout;
  int showstatuscode;
  long fieldindex_host;
  long fieldindex_ext;
  long fieldindex_pin;
  char* current_host;
  char* current_ext;
  char* current_pin;
};

void csv_callback_field (void* value, size_t valuelen, void* data)
{
  struct csv_callback_data* callbackdata = (struct csv_callback_data*)data;
  if (!callbackdata->firstrowdone && value) {
    //determine columns with relevant fields
    if (strcasecmp(value, callbackdata->fieldname_host) == 0)
      callbackdata->fieldindex_host = callbackdata->column;
    if (strcasecmp(value, callbackdata->fieldname_ext) == 0)
      callbackdata->fieldindex_ext = callbackdata->column;
    if (strcasecmp(value, callbackdata->fieldname_pin) == 0)
      callbackdata->fieldindex_pin = callbackdata->column;
  } else if (value) {
    //get values
    if (callbackdata->column == callbackdata->fieldindex_host)
      callbackdata->current_host = (value ? strdup((char*)value) : NULL);
    else if (callbackdata->column == callbackdata->fieldindex_ext)
      callbackdata->current_ext = (value ? strdup((char*)value) : NULL);
    else if (callbackdata->column == callbackdata->fieldindex_pin)
      callbackdata->current_pin = (value ? strdup((char*)value) : NULL);
  }
  callbackdata->column++;
}

void csv_callback_eol (int c, void* data)
{
  struct csv_callback_data* callbackdata = (struct csv_callback_data*)data;
  if (!callbackdata->firstrowdone) {
    callbackdata->firstrowdone++;
  } else if (callbackdata->current_host && *(callbackdata->current_host)) {
    //perform action for device listed on processed line
    long statuscode = polycom_vvx_action(callbackdata->cmd, callbackdata->current_host, (callbackdata->username ? callbackdata->username : POLYCOM_VVX_DEFAULT_USER), (callbackdata->password ? callbackdata->password : POLYCOM_VVX_DEFAULT_PASS), callbackdata->current_ext, callbackdata->current_pin, callbackdata->timeout);
    if (callbackdata->showstatuscode) {
      printf("HTTP status code from %s: ", (callbackdata->current_host ? callbackdata->current_host : ""));
      if (statuscode == NOCONNECTIONERROR)
        printf("(no connection)\n");
      else
        printf("%li\n", statuscode);
    }
    free(callbackdata->current_host);
    callbackdata->current_host = NULL;
    free(callbackdata->current_ext);
    callbackdata->current_ext = NULL;
    free(callbackdata->current_pin);
    callbackdata->current_pin = NULL;
  }
  callbackdata->column = 0;
}

//show command line help
void show_help()
{
  curl_version_info_data* curlversion = curl_version_info(CURLVERSION_NOW);
  XML_Expat_Version expatver = XML_ExpatVersionInfo();
  printf(
    "PolycomVVXControl - Version " PROGRAM_VERSION " - LGPL - Brecht Sanders (2015-2016)\n" \
    "\tlibraries used: libcurl %s, expat %i.%i.%i, libcsv %i.%i.%i\n" \
    "A command line utility for remote controlling Polycom VVX IP Phones\n" \
    "Usage:  PolycomVVXControl [-?|-h]\n" \
    "        PolycomVVXControl -a address [-u user] [-w pass] [-e extension] [-p pincode] [-s] command\n" \
    "        PolycomVVXControl -r ip1 ipN [-u user] [-w pass] [-e extension] [-p pincode] [-s] command\n" \
    "        PolycomVVXControl -c file -a address [-u user] [-w pass] [-e extension] [-p pincode] [-s] command\n" \
    "Parameters:\n" \
    "  -? | -h     \tshow command line help\n" \
    "  -a address  \tIP address or hostname of Polycom VVX IP Phone\n" \
    "  -r ip1 ipN  \tscan IP range for Polycom VVX IP Phones\n" \
    "  -u user     \tusername for phone's web interface (default is " POLYCOM_VVX_DEFAULT_USER ")\n" \
    "  -w pass     \tpassword for phone's web interface (default is " POLYCOM_VVX_DEFAULT_PASS ")\n" \
    "  -e extension\textension (needed for signin command)\n" \
    "  -p pincode  \tPIN code (needed for signin command)\n" \
    "  -t seconds  \tconnection timeout in seconds (default is 10)\n" \
    "  -s          \tshow HTTP result code\n" \
    "  -c file     \tCSV file to process\n" \
    "              \tin this mode -a/-e/-p specify column names\n" \
    "Commands:\n" \
    "  deviceinfo  \tget device information\n" \
    "  lyncstatus  \tget lync sign in status\n" \
    "  lyncxml     \tget lync status XML\n" \
    "  lyncinfo    \tget lync status information\n" \
    "  signin      \tsign in with specified extension and PIN code\n" \
    "  signout     \tsign out\n" \
    "  reboot      \treboot\n" \
    "  factoryreset\treset to factory default (warning: settings will be lost)\n" \
    "\n",
    (curlversion ? curlversion->version : curl_version()),
    expatver.major, expatver.minor, expatver.micro,
    CSV_MAJOR, CSV_MINOR, CSV_RELEASE
  );
}

//set variable to string parameter following flag and advance index
#define GET_ARG_STRING(var) \
  if (argv[i][2]) \
    param = argv[i] + 2; \
  else if (i + 1 < argc && argv[i + 1]) \
    param = argv[++i]; \
  if (!param) \
    paramerror++; \
  else \
    var = param;

//set variable to integer parameter following flag and advance index
#define GET_ARG_INT(var) \
  if (argv[i][2]) \
    param = argv[i] + 2; \
  else if (i + 1 < argc && argv[i + 1]) \
    param = argv[++i]; \
  if (!param) \
    paramerror++; \
  else \
    var = atoi(param);

//set boolean to true and advance index
#define GET_ARG_FLAG(var) \
  if (argv[i][2]) \
    paramerror++; \
  else \
    var = 1;

int main (int argc, char *argv[])
{
  const char* host = NULL;
  const char* username = NULL;
  const char* password = NULL;
  const char* ext = NULL;
  const char* pin = NULL;
  int timeout = 10;
  int showstatuscode = 0;
	struct in_addr iprangefirst = {.s_addr = INADDR_NONE};
	struct in_addr iprangelast = {.s_addr = INADDR_NONE};
  FILE* csvfile = NULL;
  int cmd = CMD_NONE;

  //process command line parameters
  {
    int i = 0;
    char* param;
    int paramerror = 0;
    while (!paramerror && ++i < argc) {
      if (argv[i][0] == '/' || argv[i][0] == '-') {
        param = NULL;
        switch (tolower(argv[i][1])) {
          case '?' :
          case 'h' :
            show_help();
            return 0;
          case 'a' :
            GET_ARG_STRING(host)
            break;
          case 'r' :
            {
              GET_ARG_STRING(param)
              if (paramerror == 0) {
                iprangefirst.s_addr = inet_addr(param);
                if (i + 1 < argc && argv[i + 1]) {
                  param = argv[++i];
                  iprangelast.s_addr = inet_addr(param);
                }
              }
            }
            break;
          case 'u' :
            GET_ARG_STRING(username)
            break;
          case 'w' :
            GET_ARG_STRING(password)
            break;
          case 'e' :
            GET_ARG_STRING(ext)
            break;
          case 'p' :
            GET_ARG_STRING(pin)
            break;
          case 't' :
            GET_ARG_INT(timeout)
            break;
          case 's' :
            GET_ARG_FLAG(showstatuscode)
            break;
          case 'c' :
            {
              char* filename = NULL;
              GET_ARG_STRING(filename)
              if (!filename || !*filename) {
                paramerror++;
              } else if (strcmp(filename, "-") == 0) {
                csvfile = stdin;
              } else {
                if ((csvfile = fopen(filename, "rb")) == NULL) {
                  fprintf(stderr, "Error reading file: %s\n", filename);
                  return 2;
                }
              }
            }
            break;
          default :
            paramerror++;
            break;
        }
      } else if (cmd != CMD_NONE) {
        paramerror++;
      } else if (strcasecmp(argv[i], "deviceinfo") == 0) {
        cmd = CMD_DEVICEINFO;
      } else if (strcasecmp(argv[i], "lyncstatus") == 0) {
        cmd = CMD_LYNCSIGNSTATUS;
      } else if (strcasecmp(argv[i], "lyncxml") == 0) {
        cmd = CMD_LYNCXMLSTATUS;
      } else if (strcasecmp(argv[i], "lyncinfo") == 0) {
        cmd = CMD_LYNCINFO;
      } else if (strcasecmp(argv[i], "signin") == 0) {
        cmd = CMD_SIGNIN;
      } else if (strcasecmp(argv[i], "signout") == 0) {
        cmd = CMD_SIGNOUT;
      } else if (strcasecmp(argv[i], "reboot") == 0) {
        cmd = CMD_REBOOT;
      } else if (strcasecmp(argv[i], "factoryreset") == 0) {
        cmd = CMD_FACTORYRESET;
      }
    }
    if (paramerror || argc <= 1 || cmd == CMD_NONE || (!host && (iprangefirst.s_addr == INADDR_NONE || iprangelast.s_addr == INADDR_NONE)) || (cmd == CMD_SIGNIN && (!ext || !pin))) {
      if (paramerror)
        fprintf(stderr, "Invalid command line parameters\n");
      show_help();
      return 1;
    }
  }

  //perform requested action
  if (iprangefirst.s_addr != INADDR_NONE && iprangelast.s_addr != INADDR_NONE) {
    //perform requested action for a range of IP addresses
    struct in_addr currentaddr;
    long statuscode;
    u_long currentip = htonl(iprangefirst.s_addr);
    u_long lastip = htonl(iprangelast.s_addr);
    //reverse first and last if needed
    if (lastip < currentip) {
      u_long temp = currentip;
      currentip = lastip;
      lastip = temp;
    }
    if (!username)
      username = POLYCOM_VVX_DEFAULT_USER;
    if (!password)
      password = POLYCOM_VVX_DEFAULT_PASS;
    polycom_vvx_action_common_header(cmd);
    while (currentip <= lastip) {
      currentaddr.s_addr = ntohl(currentip++);
      {
        statuscode = polycom_vvx_action(cmd, inet_ntoa(currentaddr), username, password, ext, pin, timeout);
        if (showstatuscode) {
          printf("HTTP status code: ");
          if (statuscode == NOCONNECTIONERROR)
            printf("(no connection)\n");
          else
            printf("%li\n", statuscode);
        }
      }
    }
    return 0;
  } else if (!csvfile) {
    //perform requested action for single device
    long statuscode;
    if (!username)
      username = POLYCOM_VVX_DEFAULT_USER;
    if (!password)
      password = POLYCOM_VVX_DEFAULT_PASS;
    polycom_vvx_action_common_header(cmd);
    statuscode = polycom_vvx_action(cmd, host, username, password, ext, pin, timeout);
    if (showstatuscode) {
      printf("HTTP status code: ");
      if (statuscode == NOCONNECTIONERROR)
        printf("(no connection)\n");
      else
        printf("%li\n", statuscode);
    }
    return (statuscode >= 200 && statuscode < 300 ? 0 : statuscode);
  } else {
    //perform requested action for device listed in CSV file
    struct csv_parser p;
    size_t bytes_read;
    char buf[CSV_BUFFER_SIZE];
    struct csv_callback_data csvdata = {
      .firstrowdone = 0,
      .column = 0,
      .cmd = cmd,
      .fieldname_host = host,
      .username = username,
      .password = password,
      .fieldname_ext = ext,
      .fieldname_pin = pin,
      .timeout = 10,
      .showstatuscode = showstatuscode,
      .fieldindex_host = -1,
      .fieldindex_ext = -1,
      .fieldindex_pin = -1,
      .current_host = NULL,
      .current_ext = NULL,
      .current_pin = NULL
    };
    polycom_vvx_action_common_header(cmd);
    csv_init(&p, CSV_APPEND_NULL | CSV_EMPTY_IS_NULL);
    while ((bytes_read = fread(buf, 1, CSV_BUFFER_SIZE, csvfile)) > 0) {
      if (csv_parse(&p, buf, bytes_read, csv_callback_field, csv_callback_eol, &csvdata) != bytes_read) {
        fprintf(stderr, "Error while parsing file: %s\n", csv_strerror(csv_error(&p)));
      }
    }
    csv_fini(&p, csv_callback_field, csv_callback_eol, &csvdata);
    fclose(csvfile);
    return 0;
  }
}
