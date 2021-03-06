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

  P_UnixUDPConnection.h
  Unix UDPConnection implementation
  
  
 ****************************************************************************/
#ifndef __UNIXUDPCONNECTION_H_
#define __UNIXUDPCONNECTION_H_

#ifndef _IOCORE_WIN32

#include "P_UDPConnection.h"
class UnixUDPConnection:public UDPConnectionInternal
{
public:
  void init(int fd);
  void setEthread(EThread * e);
  void errorAndDie(int e);
  int callbackHandler(int event, void *data);

  Link<UnixUDPConnection> polling_link;
  Link<UnixUDPConnection> callback_link;
  SLink<UnixUDPConnection> newconn_alink;

  InkAtomicList inQueue;
  int onCallbackQueue;
  Action *callbackAction;
  EThread *ethread;
  struct epoll_data_ptr ep;

  UnixUDPConnection(int fd);
  virtual ~ UnixUDPConnection();
private:
  int m_errno;
  virtual void UDPConnection_is_abstract() {};
};

inline
UnixUDPConnection::UnixUDPConnection(int fd)
  : onCallbackQueue(0)
  , callbackAction(NULL)
  , ethread(NULL)
  , m_errno(0)
{
  fd = fd;
  UDPPacketInternal p;
  ink_atomiclist_init(&inQueue, "Incoming UDP Packet queue", (char *) &p.alink.next - (char *) &p);
  SET_HANDLER(&UnixUDPConnection::callbackHandler);
}

inline void
UnixUDPConnection::init(int fd)
{
  fd = fd;
  onCallbackQueue = 0;
  callbackAction = NULL;
  ethread = NULL;
  m_errno = 0;

  UDPPacketInternal p;
  ink_atomiclist_init(&inQueue, "Incoming UDP Packet queue", (char *) &p.alink.next - (char *) &p);
  SET_HANDLER(&UnixUDPConnection::callbackHandler);
}

inline void
UnixUDPConnection::setEthread(EThread * e)
{
  ethread = e;
}

inline void
UnixUDPConnection::errorAndDie(int e)
{
  m_errno = e;
}

INK_INLINE void
UDPConnection::Release()
{
  UnixUDPConnection *p = (UnixUDPConnection *) this;
  PollCont *pc = get_UDPPollCont(p->ethread);

#if defined(USE_EPOLL)
  struct epoll_event ev;
  epoll_ctl(pc->pollDescriptor->epoll_fd, EPOLL_CTL_DEL, getFd(), &ev);
#elif defined(USE_KQUEUE)
  struct kevent ev[2];
  EV_SET(&ev[0], getFd(), EVFILT_READ, EV_DELETE, 0, 0, NULL);
  EV_SET(&ev[1], getFd(), EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
  kevent(pc->pollDescriptor->kqueue_fd, &ev[0], 2, NULL, 0, NULL);
#endif

  if (ink_atomic_increment(&p->refcount, -1) == 1) {
    ink_debug_assert(p->callback_link.next == NULL);
    ink_debug_assert(p->callback_link.prev == NULL);
    ink_debug_assert(p->polling_link.next == NULL);
    ink_debug_assert(p->polling_link.prev == NULL);
    ink_debug_assert(p->newconn_alink.next == NULL);

    delete this;
  }
}

INK_INLINE Action *
UDPConnection::recv(Continuation * c)
{
  UnixUDPConnection *p = (UnixUDPConnection *) this;
  // register callback interest.
  p->continuation = c;
  ink_debug_assert(c != NULL);
  mutex = c->mutex;
  p->recvActive = 1;
  return ACTION_RESULT_NONE;
}


INK_INLINE UDPConnection *
new_UDPConnection(int fd)
{
  return (fd >= 0) ? NEW(new UnixUDPConnection(fd)) : 0;
}

#endif //_IOCORE_WIN32
#endif //__UNIXUDPCONNECTION_H_
