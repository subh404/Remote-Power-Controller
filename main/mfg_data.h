#ifndef _MGF_DATA_H_
#define _MGF_DATA_H_

char *mfg_data_get_device_id(void);
char *mfg_data_get_ca_cert(void);
char *mfg_data_get_client_cert(void);
char *mfg_data_get_client_key(void);
char *mfg_data_get_server_uri(void);
void  mfg_data_init(void);

#endif /* _MGF_DATA_H_ */