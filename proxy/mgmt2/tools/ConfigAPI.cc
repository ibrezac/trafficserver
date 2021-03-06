/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#if (HOST_OS == linux) || (HOST_OS == sunos)

#include "ConfigAPI.h"
#include "SysAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <string.h>
#include "inktomi++.h"
#include "CoreAPI.h"

#include "../utils/XmlUtils.h"
#include "../../../libinktomi++/SimpleTokenizer.h"
#include "../api2/include/INKMgmtAPI.h"

// TODO: consolidate location of these defaults
#define DEFAULT_ROOT_DIRECTORY            PREFIX
#define DEFAULT_LOCAL_STATE_DIRECTORY     "var/trafficserver"
#define DEFAULT_SYSTEM_CONFIG_DIRECTORY   "etc/trafficserver"
#define DEFAULT_LOG_DIRECTORY             "var/log/trafficserver"
#define DEFAULT_TS_DIRECTORY_FILE         PREFIX "/etc/traffic_server"

#define NETCONFIG_HOSTNAME  0
#define NETCONFIG_GATEWAY   1
#define NETCONFIG_DOMAIN    2
#define NETCONFIG_DNS       3
#define NETCONFIG_INTF_UP   4
#define NETCONFIG_INTF_DOWN 5

#define XML_MEMORY_ERROR 	1
#define XML_FILE_ERROR 		3
#define ERROR			-1

#ifdef DEBUG_SYSAPI
#define DPRINTF(x)  printf x
#else
#define DPRINTF(x)
#endif

////////////////////////////////////////////////////////////////
// the following "Config" functions are os independant. They rely on SysAPI to carry out
// OS settings handling and on INKMmgmtAPI to carry out TS seetings. Later on other SysAPIs
// which support other OSs can be added.
//

int
Config_GetHostname(char *hostname, size_t hostname_len)
{
  return (Net_GetHostname(hostname, hostname_len));
}

int
Config_SetHostname(char *hostname)
{
  int status;
  char old_hostname[256];
  INKInt val;
  bool rni = false;


  //printf("Inside Config_SetHostname(), hostname = %s\n", hostname);

  //validate
  ink_strncpy(old_hostname, "", sizeof(old_hostname));

  if (hostname == NULL)
    return -1;


  //System call first
  status = Net_SetHostname(hostname);
  if (status) {
    return status;
  }
  //MgmtAPI call 
  status = INKSetHostname(hostname);
  if (status) {
    // If this fails, we need to restore old machine hostname
    Net_GetHostname(old_hostname, sizeof(old_hostname));
    if (!strlen(old_hostname)) {
      DPRINTF(("Config_SetHostname: FATAL: recovery failed - failed to get old_hostname\n"));
      return -1;
    }
    DPRINTF(("Config_SetHostname: new hostname setup failed - reverting to  old hostname\n"));
    status = Net_SetHostname(old_hostname);
    if (status) {
      DPRINTF(("Config_SetHostname: FATAL: failed reverting to old hostname\n"));
      return status;
    }

    return -1;
  }
  //check if rmserver is installed
  INKRecordGetInt("proxy.config.rni.enabled", &val);
  rni = val;
  if (rni) {
    DPRINTF(("Config_SetHostname: calling INKSetRmRealm\n"));
    status = INKSetRmRealm(hostname);
    //status = rm_change_hostname(hostname); This is the old way
    if (status) {
      // If this fails, we need to restore old machine hostname
      // at this poing we already have TS and network configured

      Net_GetHostname(old_hostname, sizeof(old_hostname));
      if (!strlen(old_hostname)) {
        DPRINTF(("Config_SetHostname: FATAL: recovery failed - failed to get old_hostname\n"));
        return -1;
      }
      DPRINTF(("Config_SetHostname: new hostname setup failed - reverting to  old hostname\n"));
      status = Net_SetHostname(old_hostname);
      if (status) {
        DPRINTF(("Config_SetHostname: FATAL: failed reverting to old hostname - network\n"));
        return status;
      }
      status = INKSetHostname(old_hostname);
      if (status) {
        DPRINTF(("Config_SetHostname: FATAL: failed reverting to old hostname - TS\n"));
        return status;
      }
      return -1;
    }
    DPRINTF(("Config_SetHostname: calling rm_start_proxy\n"));
    status = rm_start_proxy();

    if (status) {
      DPRINTF(("Config_SetHostname: Failed starting rm proxy in rm_start_proxy\n"));
    }
  }

  return status;
}

int
Config_GetDefaultRouter(char *router, size_t len)
{

  return (Net_GetDefaultRouter(router, len));
}

int
Config_SetDefaultRouter(char *router)
{
  int status;
  char old_router[80];

  //validate
  if (router == NULL) {
    return -1;
  }
  status = Config_GetDefaultRouter(old_router, sizeof(old_router));
  if (status) {
    DPRINTF(("Config_SetDefaultRouter: Couldn't read old router name\n"));
    ink_strncpy(old_router, "", sizeof(old_router));
  }

  DPRINTF(("Config_SetDefaultRouter: router %s\n", router));

  status = Net_SetDefaultRouter(router);

  DPRINTF(("Config_SetDefaultRouter: Net_SetDefaultRouter returned %d\n", status));

  if (status) {
    return status;
  }

  status = INKSetGateway(router);
  DPRINTF(("Config_SetDefaultRouter: INKSetGateway returned %d\n", status));
  if (status) {
    if (old_router == NULL) {
      DPRINTF(("Config_SetDefaultRouter: FATAL: Couldn't revert to old router - no old router name%s\n", old_router));
      return -1;
    }
    //try to revert to old router
    status = Net_SetDefaultRouter(old_router);
    if (status) {
      DPRINTF(("Config_SetDefaultRouter: FATAL: Couldn't revert to old router %s\n", old_router));
    }
    return -1;
  }
  return status;
}

int
Config_GetDomain(char *domain, size_t domain_len)
{
  return (Net_GetDomain(domain, domain_len));
}

int
Config_SetDomain(char *domain)
{
  int status;
  char old_domain[80];

  status = Config_GetDomain(old_domain, sizeof(old_domain));
  if (status) {
    DPRINTF(("Config_SetDomain: Couldn't retrieve old domain\n"));
    ink_strncpy(old_domain, "", sizeof(old_domain));
  }
  status = Net_SetDomain(domain);
  if (status) {
    return status;
  }
  status = INKSetSearchDomain(domain);
  if (status) {
    //rollback
    if (old_domain == NULL) {
      DPRINTF(("Config_SetDomain: FATAL: no domain to revert to\n"));
      return status;
    }
    status = Net_SetDomain(old_domain);
    if (status) {
      DPRINTF(("Config_SetDomain: FATAL: couldn't revert to old domain\n"));
      return status;
    }
    return -1;
  }
  return status;
}

int
Config_GetDNS_Servers(char *dns, size_t dns_len)
{
  return (Net_GetDNS_Servers(dns, dns_len));
}

int
Config_SetDNS_Servers(char *dns)
{
  int status;
  char old_dns[80];

  DPRINTF(("Config_SetDNS_Servers: dns %s\n", dns));

  status = Config_GetDNS_Servers(old_dns, sizeof(old_dns));
  if (status) {
    DPRINTF(("Config_SetDNS_Servers: falied to retrieve old dns name\n"));
    ink_strncpy(old_dns, "", sizeof(old_dns));
  }

  status = Net_SetDNS_Servers(dns);
  if (status) {
    return status;
  }
  status = INKSetDNSServers(dns);
  if (status) {
    //if we fail we try to revert to the old dns name??
    if (old_dns == NULL) {
      DPRINTF(("Config_SetDNS_Servers: FATAL: falied to retrieve old dns name\n"));
      return -1;
    }

    status = Net_SetDNS_Servers(old_dns);
    if (status) {
      DPRINTF(("Config_SetDNS_Servers: FATAL: falied to revert to old dns name\n"));
      return status;
    }
    return -1;
  }
  return status;
}

int
Config_GetDNS_Server(char *server, size_t server_len, int no)
{
  return (Net_GetDNS_Server(server, server_len, no));
}

int
Config_GetNetworkIntCount()
{
  return (Net_GetNetworkIntCount());
}

int
Config_GetNetworkInt(int int_num, char *interface, size_t interface_len)
{
  return (Net_GetNetworkInt(int_num, interface, interface_len));
}

int
Config_GetNIC_Status(char *interface, char *status, size_t status_len)
{
  return (Net_GetNIC_Status(interface, status, status_len));
}

int
Config_GetNIC_Start(char *interface, char *start, size_t start_len)
{
  return (Net_GetNIC_Start(interface, start, start_len));
}

int
Config_GetNIC_Protocol(char *interface, char *protocol, size_t protocol_len)
{
  return (Net_GetNIC_Protocol(interface, protocol, protocol_len));
}

int
Config_GetNIC_IP(char *interface, char *ip, size_t ip_len)
{
  return (Net_GetNIC_IP(interface, ip, ip_len));
}

int
Config_GetNIC_Netmask(char *interface, char *netmask, size_t netmask_len)
{
  return (Net_GetNIC_Netmask(interface, netmask, netmask_len));
}

int
Config_GetNIC_Gateway(char *interface, char *gateway, size_t gateway_len)
{
  return (Net_GetNIC_Gateway(interface, gateway, gateway_len));
}

int
Config_SetNIC_Down(char *interface)
{
  int status;
  char ip[80];

  //validate 
  if (interface == NULL) {
    return -1;
  }

  status = Net_SetNIC_Down(interface);
  if (status) {
    return status;
  }

  Config_GetNIC_IP(interface, ip, sizeof(ip));

  status = INKSetNICDown(interface, ip);
  //do we have anything to roll back to?
  if (status) {
    DPRINTF(("Config_SetNIC_down: falied to config TS for SetNIC_Down\n"));
  }
  return status;
}

int
Config_SetNIC_StartOnBoot(char *interface, char *onboot)
{

  //validate
  if ((interface == NULL) || (onboot == NULL)) {
    return -1;
  }

  return (Net_SetNIC_StartOnBoot(interface, onboot));
}

int
Config_SetNIC_BootProtocol(char *interface, char *nic_protocol)
{
  //validate
  if ((interface == NULL) || (nic_protocol == NULL)) {
    return -1;
  }

  return (Net_SetNIC_BootProtocol(interface, nic_protocol));
}

int
Config_SetNIC_IP(char *interface, char *nic_ip)
{
  //validate
  if ((interface == NULL) || (nic_ip == NULL)) {
    return -1;
  }
  return (Net_SetNIC_IP(interface, nic_ip));
}

int
Config_SetNIC_Netmask(char *interface, char *nic_netmask)
{
  //validate
  if ((interface == NULL) || (nic_netmask == NULL)) {
    return -1;
  }

  return (Net_SetNIC_Netmask(interface, nic_netmask));
}

int
Config_SetNIC_Gateway(char *interface, char *nic_gateway)
{
  //validate
  if ((interface == NULL) || (nic_gateway == NULL)) {
    return -1;
  }

  DPRINTF(("Config_SetNIC_gateway:: interface %s gateway %s\n", interface, nic_gateway));

  return (Net_SetNIC_Gateway(interface, nic_gateway));
}

int
Config_SetNIC_Up(char *interface, char *onboot, char *protocol, char *ip, char *netmask, char *gateway)
{
  int status;
  char old_ip[80];
  INKInt val;
  bool rni = false;

  Config_GetNIC_IP(interface, old_ip, sizeof(old_ip));

  if (onboot == NULL || ip == NULL || netmask == NULL) {
    return -1;
  }
  status = Net_SetNIC_Up(interface, onboot, protocol, ip, netmask, gateway);
  if (status) {
    DPRINTF(("Config_SetNIC_Up: Failed to set NIC up\n"));
    return status;
  }

  DPRINTF(("Config_SetNIC_Up: calling INKSetNICUp \n"));
  //Rollback to keep consistent with CLI and snapshot 
  status =
    INKSetNICUp(interface, strcmp(protocol, "dhcp") != 0, ip, old_ip, netmask, strcmp(onboot, "onboot") == 0, gateway);

  if (status) {
    DPRINTF(("Config_SetNIC_Up: INKSetNICUp returned %d\n", status));
    //roll back??
    return status;
  }
  //check if real proxy is installed
  INKRecordGetInt("proxy.config.rni.enabled", &val);
  rni = val;

  if ((strcmp(interface, "eth0") == 0) && rni) {
    DPRINTF(("Config_SetNIC_Up: calling INKSetRmPNA_RDT_IP %s\n", ip));

    status = INKSetRmPNA_RDT_IP(ip);
    //status = rm_change_ip(1, &ip); This is the old way
    if (status) {
      DPRINTF(("Config_SetNIC_Up: Failed chaging ip for rm proxy\n"));  //roll back?
      return -1;
    }
    status = rm_start_proxy();
    if (status) {
      DPRINTF(("Config_SetNIC_Up: FATAL: Failed starting rm_proxy after ip change - %s\n", ip));        //roll back?
      return -1;
    }
  }

  return status;
}

int
Config_GetTime(char *hour, const size_t hourSize, char *minute, const size_t minuteSize, char *second,
               const size_t secondSize)
{
  int status;

  status = Time_GetTime(hour, hourSize, minute, minuteSize, second, secondSize);
  return status;
}

int
Config_SetTime(bool restart, char *hour, char *minute, char *second)
{
  int status;

  if (hour == NULL || minute == NULL || second == NULL) {
    return -1;
  }

  status = Time_SetTime(restart, hour, minute, second);
  return status;
}

int
Config_GetDate(char *month, const size_t monthSize, char *day, const size_t daySize, char *year, const size_t yearSize)
{
  int status;

  status = Time_GetDate(month, monthSize, day, daySize, year, yearSize);
  return status;
}

int
Config_SetDate(bool restart, char *month, char *day, char *year)
{
  int status;

  if (month == NULL || day == NULL || year == NULL) {
    return -1;
  }

  status = Time_SetDate(restart, month, day, year);
  return status;
}

int
Config_SortTimezone(void)
{
  int status;

  status = Time_SortTimezone();
  return status;
}

int
Config_GetTimezone(char *timezone, size_t timezone_len)
{
  int status;

  status = Time_GetTimezone(timezone, timezone_len);
  return status;
}

int
Config_SetTimezone(bool restart, char *timezone)
{
  int status;

  if (timezone == NULL) {
    return -1;
  }

  status = Time_SetTimezone(restart, timezone);
  return status;
}

int
Config_GetNTP_Servers(char *server, size_t server_len)
{
  int status;

  status = Time_GetNTP_Servers(server, server_len);
  return status;
}

int
Config_SetNTP_Servers(bool restart, char *server)
{
  int status;

  if (server == NULL) {
    return -1;
  }
  status = Time_SetNTP_Servers(restart, server);
  return status;
}

int
Config_GetNTP_Server(char *server, size_t server_len, int no)
{
  int status;

  status = Time_GetNTP_Server(server, server_len, no);
  return status;
}

int
Config_SaveVersion(char *file)
{
  XMLDom netConfigXML2;

  netConfigXML2.setNodeName("APPLIANCE_CONFIG");

  XMLNode *child2 = new XMLNode;
  char *parentAttributes3[3];
  size_t len;
  len = sizeof("type") + 1;
  parentAttributes3[0] = new char[len];
  snprintf(parentAttributes3[0], len, "%s", "type");
  len = sizeof("Version") + 1;
  parentAttributes3[1] = new char[len];
  snprintf(parentAttributes3[1], len, "%s", "Version");
  parentAttributes3[2] = '\0';
  child2->setAttributes(parentAttributes3);
  child2->setNodeName("CONFIG_TYPE");

  INKString TMVersion = NULL;
  if (INKRecordGetString("proxy.node.version.manager.short", &TMVersion) == INK_ERR_OKAY) {
    XMLNode *VersionString = new XMLNode;
    if (TMVersion != NULL) {
      VersionString->setNodeName("VersionString");
      VersionString->setNodeValue(TMVersion);
      child2->AppendChild(VersionString);
    } else {
      delete VersionString;
    }
  }
  netConfigXML2.AppendChild(child2);
  netConfigXML2.SaveToFile(file);
  return 0;
}


#if (HOST_OS == linux)
int
Config_SNMPSetUp(char *sys_location, char *sys_contact, char *sys_name, char *authtrapenable, char *trap_community,
                 char *trap_host)
{
  if ((sys_location == NULL) || (sys_contact == NULL) || (sys_name == NULL) || (authtrapenable == NULL) ||
      (trap_community == NULL) || (trap_host == NULL)) {
    DPRINTF(("Config_SNMPSetUp: %s %s %s %s %s %s\n", sys_location, sys_contact, sys_name, authtrapenable,
             trap_community, trap_host));
    return -1;
  }
  return Net_SNMPSetUp(sys_location, sys_contact, sys_name, authtrapenable, trap_community, trap_host);
}

int
Config_SNMPGetInfo(char *sys_location, size_t sys_location_len, char *sys_contact, size_t sys_contact_len,
                   char *sys_name, size_t sys_name_len, char *authtrapenable, size_t authtrapenable_len,
                   char *trap_community, size_t trap_community_len, char *trap_host, size_t trap_host_len)
{
  int status;
  status =
    Net_SNMPGetInfo(sys_location, sys_location_len, sys_contact, sys_contact_len, sys_name, sys_name_len,
                    authtrapenable, authtrapenable_len, trap_community, trap_community_len, trap_host, trap_host_len);
  DPRINTF(("Config_SNMPGetInfo: %s %s %s %s %s %s\n", sys_location, sys_contact, sys_name, authtrapenable,
           trap_community, trap_host));
  return status;
}
#endif

int
Config_GetNTP_Status(char *status, size_t status_len)
{
  return Time_GetNTP_Status(status, status_len);
}

int
Config_SetNTP_Off(void)
{
  return Time_SetNTP_Off();
}

int
Config_User_Root(int *old_euid)
{
  return Sys_User_Root(old_euid);
}

int
Config_User_Inktomi(int euid)
{
  return Sys_User_Inktomi(euid);
}

int
Config_Grp_Root(int *old_egid)
{
  return Sys_Grp_Root(old_egid);
}

int
Config_Grp_Inktomi(int egid)
{
  return Sys_Grp_Inktomi(egid);
}

#if (HOST_OS == linux)
int
Config_DisableInterface(char *eth)
{
  return Net_DisableInterface(eth);
}
#endif

int
Config_RestoreNetConfig(char *file)
{
  // None of these are used it seems.
  //char ethX[6];
  //char ethXIP[24];
  //char ethXNM[24];
  //char ethXGW[24];
  int ret = 0;
  char *TagValue = NULL;
  bool isFloppyConfig = false;
  int activeInterface[] = { 0, 0, 0, 0, 0 };

  //this is the only way to know whether this is a floppy restore or not

  if (strstr(file, "net_config.xml") != NULL) {
    isFloppyConfig = true;
  }

  int old_euid = getuid();
  if (ret == 0) {
    if (seteuid(0))
      perror("Config_RestoreNetConfig setuid failed: ");
    if (setreuid(0, 0))
      perror("Config_RestoreNetConfig setreuid failed: ");

    XmlObject netXml;
    ret = netXml.LoadFile(file);
    if (ret == XML_FILE_ERROR) {
      printf("File %s error. Check the file path\n", file);
      return ERROR;
    } else if (ret == XML_MEMORY_ERROR) {
      printf("Could not allocate memory for parsing the xml file %s\n", file);
      return ERROR;
    }


    TagValue = netXml.getXmlTagValue("HostName");
    if (TagValue != NULL) {
      Config_SetHostname(TagValue);
      xfree(TagValue);
    }


    TagValue = netXml.getXmlTagValue("DNSSearch");
    if (TagValue != NULL) {
      Config_SetDomain(TagValue);
      xfree(TagValue);
    }

    // Check that we always have eth0. If eth0 is missing, exit.
    // Check for all others if eth0 is found
    char eth[5];
    snprintf(eth, sizeof(eth), "eth0");
    int count = 0;
    while (count < 5) {
      TagValue = netXml.getXmlTagValueAndAttribute(eth, "PerNICDefaultGateway");
      if (TagValue != NULL) {
        Config_SetNIC_Gateway(eth, TagValue);
        xfree(TagValue);
      } else if (count == 0)
        break;
      snprintf(eth, sizeof(eth), "eth%d", ++count);
    }

    // Check that we always have eth0. If eth0 is missing, exit.
    // Check for all others if eth0 is found
    snprintf(eth, sizeof(eth), "eth0");
    count = 0;
    while (count < 5) {
      TagValue = netXml.getXmlTagValueAndAttribute(eth, "InterfaceIPAddress");
      if (TagValue != NULL) {
        Config_SetNIC_IP(eth, TagValue);
        activeInterface[count] = 1;
        xfree(TagValue);
      } else if (count == 0)
        break;
      snprintf(eth, sizeof(eth), "eth%d", ++count);
    }

#if (HOST_OS == linux)
    // Clear all disabled interfaces
    snprintf(eth, sizeof(eth), "eth0");
    count = 0;
    while (count < 5) {
      if (!activeInterface[count]) {
        Config_DisableInterface(eth);
      }
      snprintf(eth, sizeof(eth), "eth%d", ++count);
    }
#endif

    // Check that we always have eth0. If eth0 is missing, exit.
    // Check for all others if eth0 is found
    snprintf(eth, sizeof(eth), "eth0");
    count = 0;
    while (count < 5) {
      TagValue = netXml.getXmlTagValueAndAttribute(eth, "InterfaceNetmask");
      if (TagValue != NULL) {
        Config_SetNIC_Netmask(eth, TagValue);
        xfree(TagValue);
      } else if (count == 0)
        break;
      snprintf(eth, sizeof(eth), "eth%d", ++count);
    }

    TagValue = netXml.getXmlTagValue("DefaultGateway");
    if (TagValue != NULL) {
      Config_SetDefaultRouter(TagValue);
      xfree(TagValue);
    }


    TagValue = netXml.getXmlTagValue("DNSServer");
    if (TagValue != NULL) {
      Config_SetDNS_Servers(TagValue);
      xfree(TagValue);
    }

    TagValue = netXml.getXmlTagValue("NTPServers");
    if (TagValue != NULL) {
      Config_SetNTP_Servers(0, TagValue);
      xfree(TagValue);
    }

#if (HOST_OS == linux)
    char *sys_location = netXml.getXmlTagValue("SNMPSysLocation");
    char *sys_contact = netXml.getXmlTagValue("SNMPSysContact");
    char *sys_name = netXml.getXmlTagValue("SNMPSysName");
    char *authtrapenable = netXml.getXmlTagValue("SNMPauthtrapenable");
    char *trap_community = netXml.getXmlTagValue("SNMPTrapCommunity");
    char *trap_host = netXml.getXmlTagValue("SNMPTrapHost");
    Net_SNMPSetUp(sys_location, sys_contact, sys_name, authtrapenable, trap_community, trap_host);
    if (sys_location != NULL)
      xfree(sys_location);
    if (sys_contact != NULL)
      xfree(sys_contact);
    if (sys_name != NULL)
      xfree(sys_name);
    if (authtrapenable != NULL)
      xfree(authtrapenable);
    if (trap_community != NULL)
      xfree(trap_community);
    if (trap_host != NULL)
      xfree(trap_host);
#endif

    // Get Admin GUI encrypted password.
    char *e_gui_passwd;
    INKActionNeedT action_need, top_action_req = INK_ACTION_UNDEFINED;
    char *mail_address = netXml.getXmlTagValue("MailAddress");
    if (mail_address != NULL) {
      if (MgmtRecordSet("proxy.config.alarm_email", mail_address, &action_need) != INK_ERR_OKAY) {
        DPRINTF(("Config_FloppyNetRestore: failed to set new mail_address %s!\n", mail_address));
      } else {
        if (action_need < top_action_req)       // a more "severe" action is needed...
          top_action_req = action_need;
        DPRINTF(("Config_FloppyNetRestore: set new mail_address %s!\n", mail_address));
      }
      xfree(mail_address);
    }
    //Admin GUI passwd setting
    char *gui_passwd = netXml.getXmlTagValue("AdminGUIPasswd");
    if (gui_passwd != NULL) {
      if (INKEncryptPassword(gui_passwd, &e_gui_passwd) != INK_ERR_OKAY) {
        DPRINTF(("Config_FloppyNetRestore: failed to encrypt passwd %s!\n", gui_passwd));
      } else {
        if (MgmtRecordSet("proxy.config.admin.admin_password", e_gui_passwd, &action_need) != INK_ERR_OKAY) {
          DPRINTF(("Config_FloppyNetRestore: failed to set new passwd %s!\n", gui_passwd));
        } else {
          if (action_need < top_action_req)     // a more "severe" action is needed...
            top_action_req = action_need;
          DPRINTF(("Config_FloppyNetRestore: set new passwd %s!\n", gui_passwd));
        }
      }
      xfree(gui_passwd);
      if (e_gui_passwd)
        xfree(e_gui_passwd);
    }
    // Make sure this is the last entry in these series. We restart traffic server here and hence
    // should be done at the very end.
    TagValue = netXml.getXmlTagValue("TimeZone");
    if (TagValue != NULL) {
      //This is the last one - here we restart TM if it is not floppy configuration
      if (!isFloppyConfig) {
        Time_SetTimezone(true, TagValue);
      } else {
        Time_SetTimezone(false, TagValue);
      }
    }
    xfree(TagValue);
  }

  setreuid(old_euid, old_euid); //happens only for floppy config
  return 0;
}


int
Config_SaveNetConfig(char *file)
{
  // None of these are used it seems ...
  //char ethX[6];
  //char ethXIP[24];
  //char ethXNM[24];
  //char ethXGW[24];
  XMLDom netConfigXML;

  netConfigXML.setNodeName("APPLIANCE_CONFIG");

  XMLNode *child2 = new XMLNode;
  char *parentAttributes3[3];
  size_t len;
  len = sizeof("type") + 1;
  parentAttributes3[0] = new char[len];
  snprintf(parentAttributes3[0], len, "%s", "type");
  len = sizeof("Version") + 1;
  parentAttributes3[1] = new char[len];
  snprintf(parentAttributes3[1], len, "%s", "Version");
  parentAttributes3[2] = '\0';
  child2->setAttributes(parentAttributes3);
  child2->setNodeName("CONFIG_TYPE");

  INKString TMVersion = NULL;
  if (INKRecordGetString("proxy.node.version.manager.short", &TMVersion) == INK_ERR_OKAY) {
    XMLNode *VersionString = new XMLNode;
    if (TMVersion != NULL) {
      VersionString->setNodeName("VersionString");
      VersionString->setNodeValue(TMVersion);
      child2->AppendChild(VersionString);
    } else {
      delete VersionString;
    }
  }


  XMLNode *child = new XMLNode;
  char *parentAttributes[3];
  len = sizeof("type") + 1;
  parentAttributes[0] = new char[len];
  snprintf(parentAttributes[0], len, "%s", "type");
  len = sizeof("NW Settings") + 1;
  parentAttributes[1] = new char[len];
  snprintf(parentAttributes[1], len, "%s", "NW Settings");
  parentAttributes[2] = '\0';
  child->setAttributes(parentAttributes);
  child->setNodeName("CONFIG_TYPE");

  XMLNode *HostName = new XMLNode;
  char NWHostName[256];
  Net_GetHostname(NWHostName, sizeof(NWHostName));
  if (NWHostName != NULL) {
    HostName->setNodeName("HostName");
    HostName->setNodeValue(NWHostName);
    child->AppendChild(HostName);
  }

  XMLNode *DefaultGateway = new XMLNode;
  char NWDefaultGateway[256];
  Net_GetDefaultRouter(NWDefaultGateway, sizeof(NWDefaultGateway));
  if (NWDefaultGateway != NULL) {
    DefaultGateway->setNodeName("DefaultGateway");
    DefaultGateway->setNodeValue(NWDefaultGateway);
    child->AppendChild(DefaultGateway);
  }

  char Int[5];
  char NWPerNICDefaultGateway[256];
  int intCount;
  len = sizeof("InterfaceName") + 1;
  for (intCount = 0; intCount < Net_GetNetworkIntCount(); intCount++) {
    XMLNode *PerNICDefaultGateway = new XMLNode;
    snprintf(Int, sizeof(Int), "eth%d", intCount);
    Net_GetNIC_Gateway(Int, NWPerNICDefaultGateway, sizeof(NWPerNICDefaultGateway));
    if (NWPerNICDefaultGateway != NULL) {
      PerNICDefaultGateway->setNodeName("PerNICDefaultGateway");
      PerNICDefaultGateway->setNodeValue(NWPerNICDefaultGateway);
      char *attributes[3];
      attributes[0] = new char[len];
      snprintf(attributes[0], len, "%s", "InterfaceName");
      attributes[1] = Int;
      attributes[2] = '\0';
      PerNICDefaultGateway->setAttributes(attributes);
      child->AppendChild(PerNICDefaultGateway);
    }
  }

  char NWInterfaceIPAddress[256];
  for (intCount = 0; intCount < Net_GetNetworkIntCount(); intCount++) {
    XMLNode *InterfaceIPAddress = new XMLNode;
    snprintf(Int, sizeof(Int), "eth%d", intCount);
    Net_GetNIC_IP(Int, NWInterfaceIPAddress, sizeof(NWInterfaceIPAddress));
    if (NWInterfaceIPAddress != NULL) {
      InterfaceIPAddress->setNodeName("InterfaceIPAddress");
      InterfaceIPAddress->setNodeValue(NWInterfaceIPAddress);
      char *attributes[3];
      attributes[0] = new char[len];
      snprintf(attributes[0], len, "%s", "InterfaceName");
      attributes[1] = Int;
      attributes[2] = '\0';
      InterfaceIPAddress->setAttributes(attributes);
      child->AppendChild(InterfaceIPAddress);
    }
  }

  char NWInterfaceNetmask[256];
  for (intCount = 0; intCount < Net_GetNetworkIntCount(); intCount++) {
    XMLNode *InterfaceNetmask = new XMLNode;
    snprintf(Int, sizeof(Int), "eth%d", intCount);
    Net_GetNIC_Netmask(Int, NWInterfaceNetmask, sizeof(NWInterfaceNetmask));
    if (NWInterfaceNetmask != NULL) {
      InterfaceNetmask->setNodeName("InterfaceNetmask");
      InterfaceNetmask->setNodeValue(NWInterfaceNetmask);
      char *attributes[3];
      attributes[0] = new char[len];
      snprintf(attributes[0], len, "%s", "InterfaceName");
      attributes[1] = Int;
      attributes[2] = '\0';
      InterfaceNetmask->setAttributes(attributes);
      child->AppendChild(InterfaceNetmask);
    }
  }

  char NWDNSSearch[512];
  Net_GetDomain(NWDNSSearch, sizeof(NWDNSSearch));
  if (NWDNSSearch != NULL) {
    XMLNode *DNSSearch = new XMLNode;
    DNSSearch->setNodeName("DNSSearch");
    DNSSearch->setNodeValue(NWDNSSearch);
    child->AppendChild(DNSSearch);
  }

   /***
     int DNSCtrlCount = 0;
     SimpleTokenizer DNS(NWDNSSearch, ' ');

     int DNSServerCount = DNS.getNumTokensRemaining();
     for(int index=0; index < DNSServerCount; index++) {
       char *  DNStokens = DNS.getNext();
       XMLNode *DNSSearch = new XMLNode;
       DNSSearch->setNodeName("DNSSearch");
       DNSSearch->setNodeValue(DNStokens);
       char *attributes[3];
       attributes[0] = new char[sizeof("DomainControllerOrder")+1];
       sprintf(attributes[0], "%s", "DomainControllerOrder");
       attributes[1] = new char[sizeof(int)+1];;
       sprintf(attributes[1], "%d", index+1);
       attributes[2] = '\0';
       DNSSearch->setAttributes(attributes);
       child->AppendChild(DNSSearch);
     }
    ***/


  char NWNameServer[512];
  Config_GetDNS_Servers(NWNameServer, sizeof(NWNameServer));
  if (NWNameServer != NULL) {
    SimpleTokenizer NS(NWNameServer, ' ');

    int DNSServerCount = NS.getNumTokensRemaining();
    for (int index = 0; index < DNSServerCount; index++) {
      char *NSTokens = NS.getNext();
      XMLNode *DNSServer = new XMLNode;
      DNSServer->setNodeName("DNSServer");
      DNSServer->setNodeValue(NSTokens);
      char *attributes[3];
      len = sizeof("DomainControllerOrder") + 1;
      attributes[0] = new char[len];
      snprintf(attributes[0], len, "%s", "DomainControllerOrder");
      len = sizeof(int) + 1;
      attributes[1] = new char[len];;
      snprintf(attributes[1], len, "%d", index + 1);
      attributes[2] = '\0';
      DNSServer->setAttributes(attributes);
      child->AppendChild(DNSServer);
    }
  }


  XMLNode *NTPServers = new XMLNode;
  char NTPServerName[256];
  Config_GetNTP_Servers(NTPServerName, sizeof(NTPServerName));
  if (NTPServerName != NULL) {
    NTPServers->setNodeName("NTPServers");
    NTPServers->setNodeValue(NTPServerName);
    child->AppendChild(NTPServers);
  }

#if (HOST_OS == linux)
  char sys_location[256];
  char sys_contact[256];
  char sys_name[256];
  char authtrapenable[256];
  char trap_community[256];
  char trap_host[256];
  Net_SNMPGetInfo(sys_location, sizeof(sys_location), sys_contact, sizeof(sys_contact), sys_name, sizeof(sys_name),
                  authtrapenable, sizeof(authtrapenable), trap_community, sizeof(trap_community), trap_host,
                  sizeof(trap_host));
  if (sys_location != NULL) {
    XMLNode *SNMPinfo = new XMLNode;
    SNMPinfo->setNodeName("SNMPSysLocation");
    SNMPinfo->setNodeValue(sys_location);
    child->AppendChild(SNMPinfo);
  }
  if (sys_contact != NULL) {
    XMLNode *SNMPinfo = new XMLNode;
    SNMPinfo->setNodeName("SNMPSysContact");
    SNMPinfo->setNodeValue(sys_contact);
    child->AppendChild(SNMPinfo);
  }
  if (sys_location != NULL) {
    XMLNode *SNMPinfo = new XMLNode;
    SNMPinfo->setNodeName("SNMPSysName");
    SNMPinfo->setNodeValue(sys_name);
    child->AppendChild(SNMPinfo);
  }
  if (sys_location != NULL) {
    XMLNode *SNMPinfo = new XMLNode;
    SNMPinfo->setNodeName("SNMPauthtrapenable");
    SNMPinfo->setNodeValue(authtrapenable);
    child->AppendChild(SNMPinfo);
  }
  if (sys_location != NULL) {
    XMLNode *SNMPinfo = new XMLNode;
    SNMPinfo->setNodeName("SNMPTrapCommunity");
    SNMPinfo->setNodeValue(trap_community);
    child->AppendChild(SNMPinfo);
  }
  if (sys_location != NULL) {
    XMLNode *SNMPinfo = new XMLNode;
    SNMPinfo->setNodeName("SNMPTrapHost");
    SNMPinfo->setNodeValue(trap_host);
    child->AppendChild(SNMPinfo);
  }
#endif

  XMLNode *child1 = new XMLNode;
  char *parentAttributes1[3];
  len = sizeof("type") + 1;
  parentAttributes1[0] = new char[len];
  snprintf(parentAttributes1[0], len, "%s", "type");
  len = sizeof("OS Settings") + 1;
  parentAttributes1[1] = new char[len];
  snprintf(parentAttributes1[1], len, "%s", "OS Settings");
  parentAttributes1[2] = '\0';
  child1->setAttributes(parentAttributes1);
  child1->setNodeName("CONFIG_TYPE");


   /***
   XMLNode *EncryptedRootPasswd = new XMLNode;
   char *NWEncryptedRootPasswd;
   Net_GetEncryptedRootPassword(&NWEncryptedRootPasswd);
     if(NWEncryptedRootPasswd) {
       EncryptedRootPasswd->setNodeName("EncryptedRootPasswd");
       EncryptedRootPasswd->setNodeValue(NWEncryptedRootPasswd);
       child1->AppendChild(EncryptedRootPasswd);
     }
   ***/

  XMLNode *TimeZone = new XMLNode;
  char NWTimeZone[256];
  Time_GetTimezone(NWTimeZone, sizeof(NWTimeZone));
  if (NWTimeZone != NULL) {
    TimeZone->setNodeName("TimeZone");
    TimeZone->setNodeValue(NWTimeZone);
    child1->AppendChild(TimeZone);
  }



  netConfigXML.AppendChild(child2);
  netConfigXML.AppendChild(child);
  netConfigXML.AppendChild(child1);
  netConfigXML.SaveToFile(file);

  return 0;
}


int
XmlObject::LoadFile(char *file)
{
  return xmlDom.LoadFile(file);
}

char *
XmlObject::getXmlTagValue(char *XmlTagName)
{
  char XmlTagValue[1024];
  ink_strncpy(XmlTagValue, "", sizeof(XmlTagValue));

  for (int parent = 0; parent < xmlDom.getChildCount(); parent++) {
    XMLNode *parentNode = xmlDom.getChildNode(parent);
    if (parentNode != NULL) {
      int XmlTagCount = parentNode->getChildCount(XmlTagName);
      for (int tagCount = 0; tagCount < XmlTagCount; tagCount++) {
        if (parentNode->getChildNode(XmlTagName, tagCount)->getNodeValue() != NULL) {
          strncat(XmlTagValue, parentNode->getChildNode(XmlTagName, tagCount)->getNodeValue(),
                  sizeof(XmlTagValue) - strlen(XmlTagValue) - 1);
          if (tagCount + 1 < XmlTagCount)
            strncat(XmlTagValue, " ", sizeof(XmlTagValue) - strlen(XmlTagValue) - 1);
        }
      }
    }
  }
  strncat(XmlTagValue, "\0", sizeof(XmlTagValue) - strlen(XmlTagValue) - 1);
  if (strlen(XmlTagValue) == 0)
    return NULL;
  return xstrdup(XmlTagValue);
}


char *
XmlObject::getXmlTagValueAndAttribute(char *XmlAttribute, char *XmlTagName)
{
  char XmlTagValue[1024];
  ink_strncpy(XmlTagValue, "", sizeof(XmlTagValue));

  for (int parent = 0; parent < xmlDom.getChildCount(); parent++) {
    XMLNode *parentNode = xmlDom.getChildNode(parent);

    int XmlTagCount = parentNode->getChildCount(XmlTagName);
    for (int tagCount = 0; tagCount < XmlTagCount; tagCount++) {
      if (parentNode->getChildNode(XmlTagName, tagCount)->getNodeValue() != NULL) {
        if ((parentNode->getChildNode(XmlTagName, tagCount)->m_nACount > 0) && (strcmp(parentNode->
                                                                                       getChildNode(XmlTagName,
                                                                                                    tagCount)->
                                                                                       m_pAList[0].pAValue,
                                                                                       XmlAttribute) == 0)) {
          strncat(XmlTagValue, parentNode->getChildNode(XmlTagName, tagCount)->getNodeValue(),
                  sizeof(XmlTagValue) - strlen(XmlTagValue) - 1);
          strncat(XmlTagValue, "\0", sizeof(XmlTagValue) - strlen(XmlTagValue) - 1);
          return xstrdup(XmlTagValue);
        }
      }
    }
  }
  return NULL;
}





int
Config_SetSMTP_Server(char *server)
{
  return (Net_SetSMTP_Server(server));
}

int
Config_GetSMTP_Server(char *server)
{
  return (Net_GetSMTP_Server(server));
}



/* helper function to umount the flopppy when we are done.
 *
 */


int
uMountFloppy(char *net_floppy_config)
{
  pid_t pid;

  if ((pid = fork()) < 0) {
    DPRINTF(("Config_FloppyNetRestore [uMountFloppy]: unable to fork()\n"));
    return 1;
  } else if (pid > 0) {         /* Parent */
    int status;
    waitpid(pid, &status, 0);

    if (status != 0) {
      DPRINTF(("Config_FloppyNetRestore [uMountFloppy]: %s done failed!\n", net_floppy_config));
      return 1;
    }
  } else {
    int res = execl(net_floppy_config,
                    "net_floppy_config",
                    "done",
                    NULL);
    return res;
  }

  return 0;
}



/* This function will use mostly available APIs to set network settings from floppy.
 * It uses the same XML file format used by the snapshot function, with added funtionality.
 * It also uses a script name net_floppy_config to make sure the floppy is mounted and has the right XML file
 *
 */

int
Config_FloppyNetRestore()
{

  FILE *ts_file, *tmp_floppy_config;
  int i = 0;
  pid_t pid;
  char buffer[1024];
  char ts_base_dir[1024];
  char floppy_config_file[1024];
  char mount_dir[1024];
  char net_floppy_config[1024]; //script file which mounts the floppy
  // None of these seems to be used ...
  //char *mail_address, *sys_location, *sys_contact, *sys_name, *authtrapenable, *trap_community, *trap_host;
  //char *gui_passwd, *e_gui_passwd;
  //INKActionNeedT action_need, top_action_req = INK_ACTION_UNDEFINED;
  int status;
  struct stat buf;
  char *env_path;

  //first mount the floppy
  // NOTE - this script is system specific, thus if you use this funciton not under LINUX, you need to provide the appropriate 
  // script for the specific OS you use

  if ((env_path = getenv("TS_ROOT"))) {
    ink_strncpy(ts_base_dir, env_path, sizeof(ts_base_dir));
  } else {
    if ((ts_file = fopen(DEFAULT_TS_DIRECTORY_FILE, "r")) == NULL) {
      ink_strncpy(ts_base_dir, "/usr/local", sizeof(ts_base_dir));
    } else {
      NOWARN_UNUSED_RETURN(fgets(buffer, 1024, ts_file));
      fclose(ts_file);
      while (!isspace(buffer[i])) {
        ts_base_dir[i] = buffer[i];
        i++;
      }
      ts_base_dir[i] = '\0';
    }
  }

  snprintf(net_floppy_config, sizeof(net_floppy_config), "%s/bin/net_floppy_config", ts_base_dir);

  if (stat(net_floppy_config, &buf) < 0) {
    DPRINTF(("Config_FloppyNetRestore: net_floppy_config does not exist - abort\n"));
    return 1;
  }

  if ((pid = fork()) < 0) {
    DPRINTF(("Config_FloppyNetRestore: unable to fork()\n"));
    return 1;
  } else if (pid > 0) {         /* Parent */
    int status;
    waitpid(pid, &status, 0);

    if (status != 0) {
      DPRINTF(("Config_FloppyNetRestore: %s do failed!\n", net_floppy_config));
      return 1;
    }
  } else {
    int res = execl(net_floppy_config,
                    "net_floppy_config",
                    "do",
                    NULL);
    return res;
  }

  //now the floppy is mounted with the right file
  // First, call the snapshot restore function
  //Here we assume for now /mnt/floppy/floppy.cnf - but we can change it
  //easily as this is written in /tmp/net_floppy_config by the script

  if ((tmp_floppy_config = fopen("/tmp/net_floppy_config", "r")) == NULL) {
    DPRINTF(("Config_FloppyNetRestore: unable to open /tmp/net_floppy_config.\n"));
    return 1;
  }

  i = 0;
  NOWARN_UNUSED_RETURN(fgets(buffer, 1024, tmp_floppy_config));
  fclose(tmp_floppy_config);
  while (!isspace(buffer[i])) {
    mount_dir[i] = buffer[i];
    i++;
  }
  mount_dir[i] = '\0';

  // Copy the net_config.xml from floppy to /tmp/net_config.xml.
  // Unmount floppy and then use /tmp/net_config.xml to restore the
  // settings. This is required as a restart of traffic_manager 
  //  might hinder unmount of floppy
  NOWARN_UNUSED_RETURN(system("rm -f /tmp/net_config.xml"));

  char xml_temp_dir[256];
  snprintf(xml_temp_dir, sizeof(xml_temp_dir), "/bin/cp -f %s/net_config.xml /tmp/net_config.xml", mount_dir);
  NOWARN_UNUSED_RETURN(system(xml_temp_dir));
  uMountFloppy(net_floppy_config);      //umount the floppy

  //sprintf(floppy_config_file, "%s/net_config.xml", mount_dir);
  snprintf(floppy_config_file, sizeof(floppy_config_file), "/tmp/net_config.xml");

/** Lock file manipulation. We should implement this
    struct stat floppyNetConfig;
    bool restoreConfig = true;
    int oldModTime;
    if(!stat(floppy_config_file, &floppyNetConfig)) {
      FILE *floppyLockFile = fopen("/home/inktomi/5.2.12/etc/trafficserver/internal/floppy.dat", "r+");
      if(floppyLockFile != NULL) {
        fscanf(floppyLockFile, "%d", &oldModTime);
        if(oldModTime == floppyNetConfig.st_mtime)
          restoreConfig = false;
        else {
          rewind(floppyLockFile);
          fprintf(floppyLockFile, "%d", floppyNetConfig.st_mtime);
        }
        fclose(floppyLockFile);
      }
    }
***/

  bool restoreConfig = true;
  if (restoreConfig) {
    status = Config_RestoreNetConfig(floppy_config_file);
    if (status) {
      DPRINTF(("Config_FloppyNetRestore: call to Config_RestoreNetConfig failed!\n"));
      //uMountFloppy(net_floppy_config); //umount the floppy
      return status;
    }
  }

  return 0;

}

#endif
