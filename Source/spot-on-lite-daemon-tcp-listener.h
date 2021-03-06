/*
** Copyright (c) 2011 - 10^10^10, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Spot-On-Lite without specific prior written permission.
**
** SPOT-ON-LITE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** SPOT-ON-LITE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _spot_on_lite_daemon_tcp_listener_h_
#define _spot_on_lite_daemon_tcp_listener_h_

#include <QTcpServer>
#include <QTimer>

class spot_on_lite_daemon;

class spot_on_lite_daemon_tcp_listener: public QTcpServer
{
  Q_OBJECT

 public:
  spot_on_lite_daemon_tcp_listener(const QString &configuration,
				   spot_on_lite_daemon *parent);
  ~spot_on_lite_daemon_tcp_listener();

 protected:
  void incomingConnection(qintptr socket_descriptor);

 private:
  QHash<pid_t, char> m_child_pids;
  QString m_configuration;
  QTimer m_purge_timer;
  QTimer m_start_timer;
  spot_on_lite_daemon *m_parent;

 private slots:
  void slot_child_died(const pid_t pid);
  void slot_purge_timeout(void);
  void slot_start_timeout(void);

 signals:
  void new_connection(const qintptr socket_descriptor,
		      const QHostAddress &address,
		      const quint16 port);
};

#endif
