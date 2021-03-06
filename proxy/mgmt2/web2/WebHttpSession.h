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

/****************************************************************************
 *
 *  WebHttpSession.h - Manage session data
 *
 * 
 ****************************************************************************/

#ifndef _WEB_HTTP_SESSION_
#define _WEB_HTTP_SESSION_

#include "P_RecCore.h"

typedef void (*WebHttpSessionDeleter) (void *data);
void InkMgmtApiCtxDeleter(void *data);

void WebHttpSessionInit();
int WebHttpSessionStore(char *key, void *data, WebHttpSessionDeleter deleter_func);
int WebHttpSessionRetrieve(char *key, void **data);
int WebHttpSessionDelete(char *key);

char *WebHttpMakeSessionKey_Xmalloc();

#ifdef OEM
struct current_session_ele
{
  char *session_id;
  time_t created;
  time_t last_access;
  char *last_state;
  bool session_status;
};

static ink_mutex current_session_mutex;

void WebHttpCurrentSessionInit();
int WebHttpCurrentSessionStore(char *key);
int WebHttpCurrentSessionRetrieve(char *key, current_session_ele ** data);
int WebHttpCurrentSessionDelete(char *key);
#endif // OEM
#endif // _WEB_HTTP_SESSION_
