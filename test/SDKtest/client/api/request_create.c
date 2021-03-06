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

/* request_create.c 
 *
 */

#include "ClientAPI.h"
#include <stdio.h>

#define TRUE 1

void
INKPluginInit(int clientid)
{
  fprintf(stderr, "*** INKRequestCreate Test for Client ***\n");
  INKFuncRegister(INK_FID_REQUEST_CREATE);
}

int
INKRequestCreate(char *origin_server_host /* return */ , int max_hostname_size,
                 char *origin_server_port /* return */ , int max_portname_size,
                 char *request_buf /* return */ , int max_request_size,
                 void **req_id /* return */ )
{

  /* if we do not fill the request_buf, the default 
   * request generated by SDKtest_client will be used
   */
  return TRUE;
}
