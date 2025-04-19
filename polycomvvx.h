#ifndef INCLUDED_POLYCOMVVX_H
#define INCLUDED_POLYCOMVVX_H

#define CURL_TIMEOUT 5
#define NOCONNECTIONERROR 9999

#define CMD_NONE            0
#define CMD_DEVICEINFO      1
#define CMD_LYNCSIGNSTATUS  2
#define CMD_LYNCXMLSTATUS   3
#define CMD_LYNCINFO        4
#define CMD_SIGNIN          5
#define CMD_SIGNOUT         6
#define CMD_REBOOT          7
#define CMD_FACTORYRESET    8

#define POLYCOM_VVX_DEFAULT_USER  "Polycom"
#define POLYCOM_VVX_DEFAULT_PASS  "456"

//perform an HTTP GET action to a Polycom VVX phone
char* polycom_vvx_get (const char* host, const char* username, const char* password, const char* urlpath, long* responsecode, long timeout);

//perform an HTTP POST action to a Polycom VVX phone
char* polycom_vvx_post (const char* host, const char* username, const char* password, const char* urlpath, const char* postdata, long* responsecode, long timeout);

//make a VVX phone sign in with PIN authentication
char* polycom_vvx_signin_pin (const char* host, const char* username, const char* password, const char* urlpath, const char* ext, const char* pin, long* responsecode, long timeout);

//display common header for actions that output data
void polycom_vvx_action_common_header (int cmd);

//perform requested action
long polycom_vvx_action (int cmd, const char* host, const char* username, const char* password, const char* ext, const char* pin, long timeout);

#endif //INCLUDED_POLYCOMVVX_H
