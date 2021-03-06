/** @file

  Private RecFile and RecPipe declarations

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

#ifndef _P_REC_COMPATIBILITY_H_
#define _P_REC_COMPATIBILITY_H_

#include "ink_platform.h"

//-------------------------------------------------------------------------
// types/defines
//-------------------------------------------------------------------------

#if defined(_WIN32)

#define REC_HANDLE_INVALID INVALID_HANDLE_VALUE
typedef HANDLE RecHandle;

#else

#define REC_HANDLE_INVALID -1
typedef int RecHandle;

#endif

//-------------------------------------------------------------------------
// RecFile
//-------------------------------------------------------------------------

RecHandle RecFileOpenR(const char *file);
RecHandle RecFileOpenW(const char *file);
void RecFileClose(RecHandle h_file);
int RecFileRead(RecHandle h_file, char *buf, int size, int *bytes_read);
int RecFileWrite(RecHandle h_file, char *buf, int size, int *bytes_written);
int RecFileImport_Xmalloc(const char *file, char **file_buf, int *file_size);
int RecFileGetSize(RecHandle h_file);
int RecFileExists(const char *file);

//-------------------------------------------------------------------------
// RecPipe
//-------------------------------------------------------------------------

RecHandle RecPipeCreate(char *base_path, char *name);
RecHandle RecPipeConnect(char *base_path, char *name);
int RecPipeClose(RecHandle h_pipe);
int RecPipeRead(RecHandle h_pipe, char *buf, int size);
int RecPipeWrite(RecHandle h_pipe, char *buf, int size);

#endif
