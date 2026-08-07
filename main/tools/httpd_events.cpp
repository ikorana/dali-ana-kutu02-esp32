
int httpd_replace_func(char *aranan, char *returned)
{
    int pp=-2;

    if (strcmp(aranan,"VER")==0) sprintf(returned,"%s",(char *)GlobalConfig.version);
    if (strcmp(aranan,"YR")==0) sprintf(returned,"%s",(char *)GlobalConfig.yer);
    if (strcmp(aranan,"LIC")==0) sprintf(returned,"%s",(char *)GlobalConfig.license);
    if (strcmp(aranan,"MQTT")==0) sprintf(returned,"%s",(char *)GlobalConfig.mqtt);
    if (strcmp(aranan,"DEVID")==0) sprintf(returned,"%d",GlobalConfig.device_id);
    if (strcmp(aranan,"LLEV")==0) sprintf(returned,"%d",GlobalConfig.log_level);
    if (strcmp(aranan,"WEB")==0) {
		if (GlobalConfig.http_start==1) strcpy(returned,"checked"); else strcpy(returned," ");
                                 }
    if (strcmp(aranan,"CHN1")==0) {
		if (GlobalConfig.kanal1==1) strcpy(returned,"checked"); else strcpy(returned," ");
                                 }
    if (strcmp(aranan,"CHN2")==0) {
		if (GlobalConfig.kanal2==1) strcpy(returned,"checked"); else strcpy(returned," ");
                                 }
    if (strcmp(aranan,"CHN3")==0) {
		if (GlobalConfig.kanal3==1) strcpy(returned,"checked"); else strcpy(returned," ");
                                 }                             
    if (strcmp(aranan,"CHN4")==0) {
		if (GlobalConfig.kanal4==1) strcpy(returned,"checked"); else strcpy(returned," ");
                                 }   

        if (strcmp(aranan,"MAC")==0) {
		if (NetworkConfig.mac_chg==1) strcpy(returned,"checked"); else strcpy(returned," ");    
	                             }
    if (strcmp(aranan,"FORMAT")==0) {
		if (GlobalConfig.disk_format==1) strcpy(returned,"checked"); else strcpy(returned," ");    
	                             } 
    if (strcmp(aranan,"MODFORMAT")==0) {
		if (GlobalConfig.mod_format==1) strcpy(returned,"checked"); else strcpy(returned," ");    
	                             }                              

    if (strcmp(aranan,"TSYNC")==0) {
			if (GlobalConfig.time_sync==1) strcpy(returned,"checked"); else strcpy(returned," ");
		                          }   
    if (strcmp(aranan,"PRJ")==0) sprintf(returned,"%d",GlobalConfig.project_number);
    if (strcmp(aranan,"BINA")==0) sprintf(returned,"%d",GlobalConfig.binaNo);
    if (strcmp(aranan,"KAT")==0) sprintf(returned,"%d",GlobalConfig.katNo);
    if (strcmp(aranan,"DAIRE")==0) sprintf(returned,"%d",GlobalConfig.daireNo);

    if (strcmp(aranan,"AI")==0) {
			if (GlobalConfig.Ai==1) strcpy(returned,"checked"); else strcpy(returned," ");
		                          }   
    if (strcmp(aranan,"PNG")==0) {
			if (GlobalConfig.ping_active==1) strcpy(returned,"checked"); else strcpy(returned," ");
		                          }

    if (strcmp(aranan,"SULA")==0) {
			if (GlobalConfig.sulama==1) strcpy(returned,"checked"); else strcpy(returned," ");
		                          }
    if (strcmp(aranan,"AYD")==0) {
			if (GlobalConfig.aydinlatma==1) strcpy(returned,"checked"); else strcpy(returned," ");
		                          }
                              
    

    if (strcmp(aranan,"OUT1")==0) {
        if (NetworkConfig.wan_type==1)
            strcpy(returned,"checked");
        else
            strcpy(returned," ");
                                }
    if (strcmp(aranan,"OUT2")==0) {
        if (NetworkConfig.wan_type==2)
            strcpy(returned,"checked");
        else
            strcpy(returned," ");
                                }
    if (strcmp(aranan,"GETIP1")==0) {
        if (NetworkConfig.ipstat==1)
            strcpy(returned,"checked");
        else
            strcpy(returned," ");
                                }
    if (strcmp(aranan,"GETIP2")==0) {
        if (NetworkConfig.ipstat==2)
            strcpy(returned,"checked");
        else
            strcpy(returned," ");
                                }

    if (strcmp(aranan,"IP")==0) sprintf(returned,"%s",(char *)NetworkConfig.ip);
    if (strcmp(aranan,"NETMASK")==0) sprintf(returned,"%s",(char *)NetworkConfig.netmask);
    if (strcmp(aranan,"GATEWAY")==0) sprintf(returned,"%s",(char *)NetworkConfig.gateway);
    if (strcmp(aranan,"DNS1")==0) sprintf(returned,"%s",(char *)NetworkConfig.dns);
    if (strcmp(aranan,"DNS2")==0) sprintf(returned,"%s",(char *)NetworkConfig.backup_dns);
    if (strcmp(aranan,"SSID")==0) sprintf(returned,"%s",(char *)NetworkConfig.ssid);
    if (strcmp(aranan,"PASS")==0) sprintf(returned,"%s",(char *)NetworkConfig.pass);
    if (strcmp(aranan,"CHAN")==0) sprintf(returned,"%d",NetworkConfig.channel);
    if (strcmp(aranan,"WRET")==0) sprintf(returned,"%d",NetworkConfig.WIFI_MAXIMUM_RETRY);
    if (strcmp(aranan,"ADMIN")==0) sprintf(returned,"%s",(char *)GlobalConfig.admin);


    if (strcmp(aranan,"BOLGE1")==0) {
        if (GlobalConfig.bolge==1)
            strcpy(returned,"checked");
        else
            strcpy(returned," ");
                                }

    if (strcmp(aranan,"BOLGE2")==0) {
        if (GlobalConfig.bolge==2)
            strcpy(returned,"checked");
        else
            strcpy(returned," ");
                                }                            

    if (strcmp(aranan,"BOLGE3")==0) {
        if (GlobalConfig.bolge==3)
            strcpy(returned,"checked");
        else
            strcpy(returned," ");
                                }                            


    if (strcmp(aranan,"BOLGE4")==0) {
        if (GlobalConfig.bolge==4)
            strcpy(returned,"checked");
        else
            strcpy(returned," ");
                                }                            

    if(strlen(returned)>0) {
        pp-=strlen(aranan);
        pp+=strlen(returned);
    } else strcpy(returned,aranan);
	    
	return pp;
}

void httpd_save(char *msg)
{
     char *buf ;
    asprintf(&buf,"%s",msg);
    char *aaa = (char*) calloc(1,50);
    char *bbb = (char*) calloc(1,50);
    int p=0;
    
    httpd.find_param(buf,"DEVID",aaa);
    p=atoi(aaa);
    GlobalConfig.device_id = p;

    httpd.find_param(buf,"LLEV",aaa);
    p=atoi(aaa);
    GlobalConfig.log_level = p;

    httpd.find_param(buf,"WEB",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.http_start = 1; else GlobalConfig.http_start = 0;

    httpd.find_param(buf,"SULA",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.sulama = 1; else GlobalConfig.sulama = 0;

    httpd.find_param(buf,"AYD",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.aydinlatma = 1; else GlobalConfig.aydinlatma = 0;



    httpd.find_param(buf,"CHN1",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.kanal1 = 1; else GlobalConfig.kanal1 = 0;

    httpd.find_param(buf,"CHN2",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.kanal2 = 1; else GlobalConfig.kanal2 = 0;

    httpd.find_param(buf,"CHN3",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.kanal3 = 1; else GlobalConfig.kanal3 = 0;

    httpd.find_param(buf,"CHN4",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.kanal4 = 1; else GlobalConfig.kanal4 = 0;

    httpd.find_param(buf,"MAC",aaa);
    if (strcmp(aaa,"on")==0) NetworkConfig.mac_chg = 1; else NetworkConfig.mac_chg = 0;

    httpd.find_param(buf,"FORMAT",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.disk_format = 1; else GlobalConfig.disk_format = 0;

    httpd.find_param(buf,"MODFORMAT",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.mod_format = 1; else GlobalConfig.mod_format = 0;

    httpd.find_param(buf,"TSYNC",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.time_sync = 1; else GlobalConfig.time_sync = 0;

    httpd.find_param(buf,"PRJ",aaa);
    p=atoi(aaa);
    GlobalConfig.project_number = p;

    httpd.find_param(buf,"AI",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.Ai = 1; else GlobalConfig.Ai = 0;

    httpd.find_param(buf,"PNG",aaa);
    if (strcmp(aaa,"on")==0) GlobalConfig.ping_active = 1; else GlobalConfig.ping_active = 0;


    httpd.find_param(buf,"BINA",aaa);
    p=atoi(aaa);
    GlobalConfig.binaNo = p;

    httpd.find_param(buf,"KAT",aaa);
    p=atoi(aaa);
    GlobalConfig.katNo = p;

    httpd.find_param(buf,"DAIRE",aaa);
    p=atoi(aaa);
    GlobalConfig.daireNo = p;

    httpd.find_param(buf,"OUT",aaa);
    p=atoi(aaa);
    NetworkConfig.wan_type = (home_wan_type_t)p;

    httpd.find_param(buf,"BOLGE",aaa);
    p=atoi(aaa);
    GlobalConfig.bolge = p;


    httpd.find_param(buf,"GETIP",aaa);
    p=atoi(aaa);
    NetworkConfig.ipstat = (home_ipstat_type_t)p;

    httpd.find_param(buf,"ADMIN",aaa);
    strcpy((char *)GlobalConfig.admin,aaa);

    httpd.find_param(buf,"LIC",aaa);
    httpd.replacechar(aaa,'+',' ');
    strcpy(bbb,httpd.urlDecode(aaa));
    strcpy((char *)GlobalConfig.license,aaa);

    httpd.find_param(buf,"MQTT",aaa);
    httpd.replacechar(aaa,'+',' ');
    strcpy(bbb,httpd.urlDecode(aaa));
    strcpy((char *)GlobalConfig.mqtt,aaa);

    httpd.find_param(buf,"IP",aaa);
    strcpy((char *)NetworkConfig.ip,aaa);

    httpd.find_param(buf,"NETMASK",aaa);
    strcpy((char *)NetworkConfig.netmask,aaa);

    httpd.find_param(buf,"GATEWAY",aaa);
    strcpy((char *)NetworkConfig.gateway,aaa);

    httpd.find_param(buf,"DNS1",aaa);
    strcpy((char *)NetworkConfig.dns,aaa);

    httpd.find_param(buf,"DNS2",aaa);
    strcpy((char *)NetworkConfig.backup_dns,aaa);

    httpd.find_param(buf,"SSID",aaa);
    httpd.replacechar(aaa,'+',' ');
    strcpy(bbb,httpd.urlDecode(aaa));
    printf("SSID %s\n",bbb);
    strcpy((char *)NetworkConfig.ssid,bbb);

    httpd.find_param(buf,"PASS",aaa);
    httpd.replacechar(aaa,'+',' ');
    strcpy(bbb,httpd.urlDecode(aaa));
    printf("PASS %s\n",bbb);
    strcpy((char *)NetworkConfig.pass,bbb);

    httpd.find_param(buf,"CHAN",aaa);
    p=atoi(aaa);
    NetworkConfig.channel = p;

    httpd.find_param(buf,"WRET",aaa);
    p=atoi(aaa);
    NetworkConfig.WIFI_MAXIMUM_RETRY = p;

    disk.write_file(NETWORK_FILE,&NetworkConfig,sizeof(NetworkConfig),0);
    disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
   

    free(bbb); free(aaa); free(buf);  
}


void httpd_handler(httpd_event_t event, httpd_msg_t *msg)
   {
       printf("HTTP Event\n");
        if (event==HTTPD_DEFAULT_RESET) {
            ESP_LOGI("MAIN", "DEFAULT RESET");  
            network_default_config();
            default_config();          
        }
        if (event==HTTPD_SAVE) {
            
            ESP_LOGI("MAIN", "SAVE %s", (char*)msg->payload);
            httpd_save((char*)msg->payload);           
        }

       if (event==HTTPD_EVENT_WS_RECV) {
           
           ESP_LOGI("MAIN", "RECV WS %s", (char*)msg->payload);
           free(msg->payload);           
       }       
   }