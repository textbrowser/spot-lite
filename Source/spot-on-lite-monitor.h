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

#ifndef _spot_on_lite_monitor_h_
#define _spot_on_lite_monitor_h_

#include <QFuture>

#include "ui_spot-on-lite-monitor.h"

class spot_on_lite_monitor: public QMainWindow
{
  Q_OBJECT

 public:
  enum Columns
    {
     ARGUMENTS = 8,
     BYTES_ACCUMULATED = 4,
     BYTES_READ = 5,
     BYTES_WRITTEN = 6,
     IP_INFORMATION = 2,
     MEMORY = 3,
     NAME = 0,
     PID = 1,
     TYPE = 7,
     ZZZ = 999
    };
  spot_on_lite_monitor(void);
  ~spot_on_lite_monitor();

 private:
  QFuture<void> m_future;
  QMap<qint64, QMap<Columns, QVariant> > m_cache;
  Ui_spot_on_lite_monitor m_ui;
  void read_statistics_database(void);

 private slots:
};

#endif
