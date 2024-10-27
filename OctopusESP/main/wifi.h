
#ifndef WIFI_H
#define WIFI_H

void wifi_init_sta(void);
void wait_for_wifi(void);
void get_mac(char* mac_str);

#endif