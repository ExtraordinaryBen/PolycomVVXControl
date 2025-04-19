#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif
#ifdef STATIC
#define CURL_STATICLIB
#endif
#include <curl/curl.h>
#include "polycomvvx.h"

#define CSV_SEPARATOR ","
#define CSV_QUOTE "\""

#define HTTP_STATUS_OK(statuscode) (statuscode >= 100 && statuscode < 400)

//format a string (like printf) into a string, the caller must free the result
char* format_string (const char* fmt, ...)
{
  va_list args;
  int len;
  char* buf = NULL;
  va_start(args, fmt);
  if ((len = vsnprintf(NULL, 0, fmt, args)) >= 0) {
    buf = (char*)malloc(len + 1);
    vsnprintf(buf, len + 1, fmt, args);
  }
  va_end(args);
  return buf;
}

//convert a string to base64 encoding, the caller must free the result
char* encode_base64 (const char* str)
{
  int done = 0;
  char base64_table[256];
  int i;
  char* buf = (char*)malloc((strlen(str) + 2) / 3 * 4 + 1);
  char* dst = buf;
  const char* src = str;
  //fill base64_table with character encodings
  for (i = 0; i < 26; i++) {
    base64_table[i] = (char)('A' + i);
    base64_table[26 + i] = (char)('a' + i);
  }
  for (i = 0; i < 10; i++) {
    base64_table[52 + i] = (char)('0' + i);
  }
  base64_table[62] = '+';
  base64_table[63] = '/';
  //encode data
  while (!done) {
    char igroup[3];
    int n;
    //read input
    igroup[0] = igroup[1] = igroup[2] = 0;
    for (n = 0; n < 3; n++) {
      if (!*src) {
        done++;
        break;
      }
      igroup[n] = *src++;
    }
    //code
    if (n > 0) {
      *dst++ = base64_table[igroup[0] >> 2];
      *dst++ = base64_table[((igroup[0] & 3) << 4) | (igroup[1] >> 4)];
      *dst++ = (n < 2 ? '=' : base64_table[((igroup[1] & 0xF) << 2) | (igroup[2] >> 6)]);
      *dst++ = (n < 3 ? '=' : base64_table[igroup[2] & 0x3F]);
    }
  }
  *dst = 0;
  return buf;
}

//callback function to add HTTP data to a dynamically (re)allocated string
size_t curl_output_callback (char* data, size_t size, size_t nmemb, void* callbackdata)
{
  size_t len;
  char** poutput = (char**)callbackdata;
  if (!*poutput) {
    len = 0;
    *poutput = (char*)malloc(size * nmemb + 1);
  } else {
    len = strlen(*poutput);
    *poutput = (char*)realloc(*poutput, len + size * nmemb + 1);
  }
  if (!*poutput)
    return 0;
  memcpy(*poutput + len, data, size * nmemb);
  (*poutput)[len + size * nmemb] = 0;
  return size * nmemb;
}

//perform an HTTP GET action to a Polycom VVX phone
char* polycom_vvx_get (const char* host, const char* username, const char* password, const char* urlpath, long* responsecode, long timeout)
{
  CURL* curl;
  CURLcode res;
  char* output = NULL;
  curl = curl_easy_init();
  if (curl) {
    char* url = format_string("https://%s%s", host, urlpath);
    char* auth = NULL;
    char* auth_base64 = NULL;
    char* authcookie = NULL;
    char* referer = NULL;
    struct curl_slist *headerlist = NULL;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, (long)1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)(timeout ? timeout : 10));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)(timeout ? timeout : 10));
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)0);
    if (username || password) {
      auth = format_string("%s:%s", username, password);
      auth_base64 = encode_base64(auth);
      authcookie = format_string("Cookie: Authorization=Basic %s", auth_base64);
      headerlist = curl_slist_append(headerlist, authcookie);
      //curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
      //curl_easy_setopt(curl, CURLOPT_USERPWD, auth);
    }
    referer = format_string("Referer: https://%s/", host);
    headerlist = curl_slist_append(headerlist, referer);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_output_callback);
    if ((res = curl_easy_perform(curl)) == CURLE_OK) {
    }
/*
char* buf;
curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &buf);
printf("Content type: %s\n", buf);
*/
    if (responsecode) {
      *responsecode = -1;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, responsecode);
    }
    curl_slist_free_all(headerlist);
    free(authcookie);
    free(auth_base64);
    free(auth);
    free(url);
    curl_easy_cleanup(curl);
  }
  return output;
}

//perform an HTTP POST action to a Polycom VVX phone
char* polycom_vvx_post (const char* host, const char* username, const char* password, const char* urlpath, const char* postdata, long* responsecode, long timeout)
{
  CURL* curl;
  CURLcode res;
  char* output = NULL;
  curl = curl_easy_init();
  if (curl) {
    char* url = format_string("https://%s%s", host, urlpath);
    char* auth = NULL;
    char* auth_base64 = NULL;
    char* authcookie = NULL;
    char* referer = NULL;
    struct curl_slist *headerlist = NULL;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, (long)1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)(timeout ? timeout : 10));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)(timeout ? timeout : 10));
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, (long)0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)0);
    if (username || password) {
      auth = format_string("%s:%s", username, password);
      auth_base64 = encode_base64(auth);
      authcookie = format_string("Cookie: Authorization=Basic %s", auth_base64);
      headerlist = curl_slist_append(headerlist, authcookie);
      //curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
      //curl_easy_setopt(curl, CURLOPT_USERPWD, auth);
    }
    referer = format_string("Referer: https://%s/", host);
    headerlist = curl_slist_append(headerlist, referer);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    if (postdata)
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_output_callback);
    if ((res = curl_easy_perform(curl)) == CURLE_OK) {
    }
    if (responsecode) {
      *responsecode = -1;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, responsecode);
    }
    curl_slist_free_all(headerlist);
    free(authcookie);
    free(auth_base64);
    free(auth);
    free(url);
    curl_easy_cleanup(curl);
  }
  return output;
}

//make a VVX phone sign in with PIN authentication
char* polycom_vvx_signin_pin (const char* host, const char* username, const char* password, const char* urlpath, const char* ext, const char* pin, long* responsecode, long timeout)
{
  char* result;
  char* postdata = format_string("authType=3&extension=%s&pin=%s", (ext ? ext : ""), (pin ? pin : ""));
  result = polycom_vvx_post(host, username, password, "/form-submit/Settings/lyncSignIn", postdata, responsecode, timeout);
  free(postdata);
  return result;
}

//#define XML_UNICODE
#include <expat.h>



struct parse_vvx_xml_struct {
  long textid;
  const char* fieldname;
};

struct xml_parse_data {
  int in_table;
  int in_cell;
  long textid;
  char* value;
  size_t valuelen;
  const struct parse_vvx_xml_struct* fields;
  size_t numfields;
  char** fieldvalues;
};

void xml_parse_element_start (void* userdata, const XML_Char* name, const XML_Char** atts)
{
  struct xml_parse_data* parsedata = (struct xml_parse_data*)userdata;
  //keep track of where we are
  if (strcasecmp(name, "table") == 0) {
    parsedata->in_table++;
  } else if (strcasecmp(name, "tr") == 0) {
    parsedata->in_cell = 0;
  } else if (strcasecmp(name, "td") == 0) {
    parsedata->in_cell++;
  } else if (strcasecmp(name, "span") == 0) {
    if (parsedata->in_cell) {
      const XML_Char** p = atts;
      while (*p) {
        if (strcasecmp(*p, "textId") == 0) {
          parsedata->textid = strtol(*++p, NULL, 10);
        } else
          p++;
        p++;
      }
    }
  }
}

void xml_parse_element_end (void* userdata, const XML_Char *name)
{
  struct xml_parse_data* parsedata = (struct xml_parse_data*)userdata;
  //keep track of where we are
  if (strcasecmp(name, "table") == 0) {
    parsedata->in_table--;
  } else if (strcasecmp(name, "tr") == 0) {
    parsedata->textid = 0;
  } else if (strcasecmp(name, "td") == 0) {
    //output data when available
    if (parsedata->textid && parsedata->in_cell == 2 && parsedata->value) {
      char* p;
      int i;
      //skip trailing blank space
      if (parsedata->valuelen > 0) {
        p = parsedata->value + parsedata->valuelen;
        while (--p != parsedata->value && isspace(*p))
          *p = 0;
      }
      //skip leading blank space
      p = parsedata->value;
      while (*p && isspace(*p))
        p++;
      //determine field name
      for (i = 0; i < parsedata->numfields; i++) {
        if (parsedata->fields[i].textid == parsedata->textid) {
          if (parsedata->fieldvalues[i])
            free(parsedata->fieldvalues[i]);
          parsedata->fieldvalues[i] = strdup(p);
          break;
        }
      }
      //clean up
      free(parsedata->value);
      parsedata->value = NULL;
      parsedata->valuelen = 0;
    }
  }
}

void xml_parse_element_data (void* userdata, const XML_Char* data, int datalen)
{
  //collect relevant data when given
  struct xml_parse_data* parsedata = (struct xml_parse_data*)userdata;
  if (parsedata->in_cell == 2 && parsedata->textid) {
    if ((parsedata->value = (char*)realloc(parsedata->value, parsedata->valuelen + datalen + 1)) != NULL) {
      memcpy(parsedata->value + parsedata->valuelen, data, datalen);
      parsedata->valuelen += datalen;
      parsedata->value[parsedata->valuelen] = 0;
    }
  }
}

void parse_vvx_xml (const char* xmldata, const struct parse_vvx_xml_struct* fields, size_t numfields)
{
  if (xmldata) {
    int i;
    struct xml_parse_data parsedata = {0, 0, 0, NULL, 0, fields, numfields, malloc(numfields * sizeof(char*))};
    for (i = 0; i < numfields; i++)
      parsedata.fieldvalues[i] = NULL;
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, &parsedata);
    XML_SetElementHandler(parser, xml_parse_element_start, xml_parse_element_end);
    XML_SetCharacterDataHandler(parser, xml_parse_element_data);
    XML_Parse(parser, xmldata, strlen(xmldata), XML_TRUE);
    XML_ParserFree(parser);
    free(parsedata.value);
    for (i = 0; i < numfields; i++) {
      if (i)
        printf(CSV_SEPARATOR);
      if (!parsedata.fieldvalues[i]) {
      } else if (strchr(parsedata.fieldvalues[i], CSV_SEPARATOR[0]) || strchr(parsedata.fieldvalues[i], CSV_QUOTE[0])) {
        char* p = parsedata.fieldvalues[i];
        printf("%s", CSV_QUOTE);
        while (*p) {
          if (*p == CSV_QUOTE[0])
            printf(CSV_QUOTE CSV_QUOTE);
          else
            putchar(*p);
          p++;
        }
        printf("%s", CSV_QUOTE);
      } else {
        printf("%s", parsedata.fieldvalues[i]);
      }
    }
    printf("\n");
    for (i = 0; i < numfields; i++)
      free(parsedata.fieldvalues[i]);
    free(parsedata.fieldvalues);
  }
}

const struct parse_vvx_xml_struct deviceinfo_fields[] = {
  { 292, "Phone Model"},
  { 293, "Part Number"},
  { 294, "MAC Address"},
  {  97, "IP Address"},
  {1023, "UC Software Version"},
  { 297, "Updater Version"},
};

const struct parse_vvx_xml_struct lyncinfo_fields[] = {
  {1157, "STSURI"},
  {1087, "AutoDiscover"},
  {1159, "Authentication Type"},
  {1160, "Registration Expiry"},
  {1161, "Media bypass"},
  {1162, "ABS Server Internal URL"},
  {1163, "ABS Server External URL"},
  {1164, "Voice Mail URI"},
  {1165, "MRAS Server"},
  {1166, "Line;tel:"},
  {1167, "Location Profile"},
  {1168, "Call Park Server URI"},
  {1169, "User SIP URI"},
  {1170, "PC to PC AV Encryption"},
  {1171, "EWS Information"},
  {1172, "Configured Exchange URL"},
  {1174, "Connected Lync Server"},
  {1175, "Dialplan"},
  {1176, "Enhanced Emergency Services"},
  {1177, "E911 String"},
  {1184, "CAC Status"},
};

//display common header for actions that output data
void polycom_vvx_action_common_header (int cmd)
{
  int i;
  switch (cmd) {
    case CMD_DEVICEINFO :
      for (i = 0; i < sizeof(deviceinfo_fields) / sizeof(struct parse_vvx_xml_struct); i++) {
        if (i)
          printf(CSV_SEPARATOR);
        printf("%s", deviceinfo_fields[i].fieldname);
      }
      printf("\n");
      break;
    case CMD_LYNCINFO :
      printf("Host" CSV_SEPARATOR);
      for (i = 0; i < sizeof(lyncinfo_fields) / sizeof(struct parse_vvx_xml_struct); i++) {
        if (i)
          printf(CSV_SEPARATOR);
        printf("%s", lyncinfo_fields[i].fieldname);
      }
      printf("\n");
      break;
  }
}

//perform requested action
long polycom_vvx_action (int cmd, const char* host, const char* username, const char* password, const char* ext, const char* pin, long timeout)
{
  char* result;
  long statuscode = NOCONNECTIONERROR;
  switch (cmd) {
    case CMD_DEVICEINFO :
      result = polycom_vvx_get(host, username, password, "/home.htm", &statuscode, timeout);
      if (result && HTTP_STATUS_OK(statuscode))
        parse_vvx_xml(result, deviceinfo_fields, sizeof(deviceinfo_fields) / sizeof(struct parse_vvx_xml_struct));
      free(result);
      break;
    case CMD_LYNCSIGNSTATUS :
      result = polycom_vvx_get(host, username, password, "/Settings/lyncSignInStatus", &statuscode, timeout);
      if (result && HTTP_STATUS_OK(statuscode))
        printf("%s\n", result);
      free(result);
      break;
    case CMD_LYNCXMLSTATUS :
      result = polycom_vvx_get(host, username, password, "/Utilities/LyncStatusXml", &statuscode, timeout);
      if (result && HTTP_STATUS_OK(statuscode))
        printf("%s", result);
      free(result);
      break;
    case CMD_LYNCINFO :
      result = polycom_vvx_get(host, username, password, "/lyncstatus.htm", &statuscode, timeout);
      if (result && HTTP_STATUS_OK(statuscode)) {
        printf("%s" CSV_SEPARATOR, host);
        parse_vvx_xml(result, lyncinfo_fields, sizeof(lyncinfo_fields) / sizeof(struct parse_vvx_xml_struct));
      }
      free(result);
      break;
    case CMD_SIGNIN :
      if (pin && *pin) {
        result = polycom_vvx_signin_pin(host, username, password, "/form-submit/Settings/lyncSignIn", ext, pin, &statuscode, timeout);
        if (result && HTTP_STATUS_OK(statuscode))
          printf("%s\n", result);
        free(result);
        break;
      }
    case CMD_SIGNOUT :
      result = polycom_vvx_post(host, username, password, "/form-submit/Settings/lyncSignOut", NULL, &statuscode, timeout);
      if (result && HTTP_STATUS_OK(statuscode))
        printf("%s\n", result);
      free(result);
      break;
    case CMD_REBOOT :
      result = polycom_vvx_post(host, username, password, "/form-submit/Reboot", NULL, &statuscode, timeout);
      if (result && HTTP_STATUS_OK(statuscode))
        printf("%s\n", result);
      free(result);
      break;
    case CMD_FACTORYRESET :
      result = polycom_vvx_post(host, username, password, "/form-submit/Utilities/restorePhoneToFactory", NULL, &statuscode, timeout);
      if (result && HTTP_STATUS_OK(statuscode))
        printf("%s\n", result);
      free(result);
      break;
    default:
      fprintf(stderr, "Unknown command\n");
      break;
  }
  return statuscode;
}

