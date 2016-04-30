#ifndef CMD_CONNECT_H
#define CMD_CONNECT_H

void set_ssl_on     ();
void set_ssl_off    ();
bool is_ssl_enabled ();

void cmd_connect    (const char *data);
void cmd_disconnect (const char *data);

#endif
