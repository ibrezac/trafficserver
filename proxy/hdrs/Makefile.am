# Makefile.am for Hdrs
#
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

AM_CPPFLAGS = \
  $(iocore_include_dirs) \
  -I$(top_srcdir)/proxy \
  -I$(top_srcdir)/proxy/mgmt2 \
  -I$(top_srcdir)/proxy/mgmt2/preparse \
  -I$(top_srcdir)/proxy/mgmt2/utils \
  -I$(top_srcdir)/proxy/mgmt2/web2 \
  -I$(top_srcdir)/proxy/hdrs \
  -I$(top_srcdir)/proxy/logging \
  -I$(top_srcdir)/libinktomi++

noinst_LIBRARIES = libhdrs.a
EXTRA_PROGRAMS = load_http_hdr 

# Http library source files.
libhdrs_a_SOURCES = \
  HdrHeap.cc \
  HdrHeap.h \
  HdrTest.cc \
  HdrTest.h \
  HdrToken.cc \
  HdrToken.h \
  HdrTSOnly.cc \
  HdrUtils.cc \
  HdrUtils.h \
  HTTP.cc \
  HTTP.h \
  HttpCompat.cc \
  HttpCompat.h \
  MIME.cc \
  MIME.h \
  SDKAllocator.cc \
  SDKAllocator.h \
  URL.cc \
  URL.h

load_http_hdr_SOURCES = \
  HdrHeap.h \
  HTTP.h \
  load_http_hdr.cc \
  MIME.h

load_http_hdr_LDADD = -L. -lhdrs \
  $(top_builddir)/libinktomi++/libinktomi++.a \
  @LIBTCL@ @LIBRT@ @LIBTHREAD@
load_http_hdr_LDFLAGS = @EXTRA_CXX_LDFLAGS@
