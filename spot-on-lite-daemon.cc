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
**    derived from Spot-On without specific prior written permission.
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

extern "C"
{
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>
}

#include <iostream>
#include <limits>

#include <QDateTime>
#include <QFileInfo>
#include <QHostAddress>
#include <QSettings>
#include <QSocketNotifier>
#include <QStringList>
#include <QtDebug>

#include "spot-on-lite-daemon.h"
#include "spot-on-lite-daemon-tcp-listener.h"

int spot_on_lite_daemon::s_signal_usr1_fd[2];
spot_on_lite_daemon *spot_on_lite_daemon::s_instance = 0;

spot_on_lite_daemon::spot_on_lite_daemon
(const QString &configuration_file_name):QObject()
{
  s_instance = this;

  if(::socketpair(AF_UNIX, SOCK_STREAM, 0, s_signal_usr1_fd))
    qFatal("spot_on_lite_daemon::spot_on_lite_daemon(): "
	   "socketpair() failure. Exiting.");

  m_configuration_file_name = configuration_file_name;
  m_maximum_accumulated_bytes = 32 * 1024 * 1024; // 32 MiB
  m_signal_usr1_socket_notifier = new QSocketNotifier
    (s_signal_usr1_fd[1], QSocketNotifier::Read, this);
  connect(m_signal_usr1_socket_notifier,
	  SIGNAL(activated(int)),
	  this,
	  SLOT(slot_signal_usr1(void)));
}

spot_on_lite_daemon::spot_on_lite_daemon(void):QObject()
{
  m_signal_usr1_socket_notifier = 0;
  s_instance = this;
}

spot_on_lite_daemon::~spot_on_lite_daemon()
{
  s_instance = 0;
}

QString spot_on_lite_daemon::child_process_file_name(void) const
{
  return m_child_process_file_name;
}

QString spot_on_lite_daemon::child_process_ld_library_path(void) const
{
  return m_child_process_ld_library_path;
}

QString spot_on_lite_daemon::log_file_name(void) const
{
  return m_log_file_name;
}

int spot_on_lite_daemon::maximum_accumulated_bytes(void) const
{
  return m_maximum_accumulated_bytes;
}

spot_on_lite_daemon *spot_on_lite_daemon::instance(void)
{
  return s_instance;
}

void spot_on_lite_daemon::log(const QString &error) const
{
  if(error.trimmed().isEmpty())
    return;

  QFile file(m_log_file_name);

  if(file.open(QIODevice::Append | QIODevice::WriteOnly))
    {
      QDateTime dateTime(QDateTime::currentDateTime());

      file.write(dateTime.toString().toStdString().data());
      file.write("\n");
      file.write(error.trimmed().toStdString().data());
      file.write("\n");
      file.close();
    }
}

void spot_on_lite_daemon::prepare_listeners(void)
{
  while(!m_listeners.isEmpty())
    if(m_listeners.first())
      m_listeners.takeFirst()->deleteLater();
    else
      m_listeners.removeFirst();

  for(int i = 0; i < m_listeners_properties.size(); i++)
    m_listeners << new spot_on_lite_daemon_tcp_listener
      (m_listeners_properties.at(i), this);
}

void spot_on_lite_daemon::process_configuration_file(bool *ok)
{
  m_child_process_file_name.clear();
  m_child_process_ld_library_path.clear();
  m_log_file_name.clear();

  QHash<QString, char> hash;
  QSettings settings(m_configuration_file_name, QSettings::IniFormat);

  foreach(QString key, settings.allKeys())
    if(key == "child_process_file")
      {
	QFileInfo fileInfo(settings.value(key).toString());

	if(fileInfo.isExecutable() &&
	   fileInfo.isFile() &&
	   fileInfo.isReadable())
	  m_child_process_file_name = fileInfo.absoluteFilePath();
	else
	  {
	    if(ok)
	      *ok = false;

	    if(fileInfo.isFile())
	      std::cerr << "spot_on_lite_daemon::"
			<< "process_configuration_file(): "
			<< "The child process file \""
			<< fileInfo.absoluteFilePath().toStdString()
			<< "\" must be a readable executable. Ignoring entry."
			<< std::endl;
	    else
	      std::cerr << "spot_on_lite_daemon::"
			<< "process_configuration_file(): "
			<< "The child process file "
			<< "is not a regular file. Ignoring entry."
			<< std::endl;
	  }
      }
    else if(key == "child_process_ld_library_path")
      m_child_process_ld_library_path =
	settings.value(key).toString().trimmed();
    else if(key == "log_file")
      {
	QFileInfo fileInfo(settings.value(key).toString());

	fileInfo = QFileInfo(fileInfo.absolutePath());

	if(!fileInfo.isDir())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The parent directory \""
		      << fileInfo.absoluteFilePath().toStdString()
		      << "\" of the log file is not a directory. "
		      << "Ignoring entry."
		      << std::endl;
	  }
	else if(!fileInfo.isWritable())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The parent directory \""
		      << fileInfo.absoluteFilePath().toStdString()
		      << "\" of the log file is not writable. "
		      << "Ignoring entry."
		      << std::endl;
	  }
	else
	  m_log_file_name = settings.value(key).toString();
      }
    else if(key.startsWith("listener"))
      {
	QStringList list
	  (settings.value(key).toString().split(",", QString::KeepEmptyParts));

	/*
	** 0 - IP Address
	** 1 - Port
	** 2 - Backlog
	** 3 - SSL Control String
	** 4 - SSL Key Size (Bits)
	** 5 - Silence Timeout (Seconds)
	*/

	int expected = 6;

	if(list.size() != expected)
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "key \""
		      << key.toStdString()
		      << "\" does not contain "
		      << expected
		      << " attributes. Ignoring entry."
		      << std::endl;
	    continue;
	  }

	for(int i = 0; i < list.size(); i++)
	  list.replace(i, list.at(i).trimmed());

	QHostAddress hostAddress(list.at(0));
	bool listenerOK = true;

	if(hostAddress.isNull())
	  {
	    listenerOK = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener \""
		      << key.toStdString()
		      << "\" IP address is invalid. Ignoring entry."
		      << std::endl;
	  }

	bool o = true;
	int port = list.at(1).toInt(&o);

	if(!o || port < 0 || port > 65535)
	  {
	    listenerOK = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener \""
		      << key.toStdString()
		      << "\" port number is invalid. Expecting a port number "
		      << "in the range [0, 65535]. Ignoring entry."
		      << std::endl;
	  }

	int backlog = list.at(2).toInt(&o);

	if(backlog < 1 || backlog > std::numeric_limits<int>::max() || !o)
	  {
	    listenerOK = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener \""
		      << key.toStdString()
		      << "\" backlog is invalid. Expecting a value "
		      << "in the range [1, "
		      << std::numeric_limits<int>::max()
		      << "]. Ignoring entry."
		      << std::endl;
	  }

	if(!list.at(4).isEmpty())
	  {
	    int keySize = list.at(4).toInt(&o);

	    if(!(keySize == 2048 || keySize == 3072 || keySize == 4096) || !o)
	      {
		listenerOK = false;

		if(ok)
		  *ok = false;

		std::cerr << "spot_on_lite_daemon::"
			  << "process_configuration_file(): The "
			  << "listener \""
			  << key.toStdString()
			  << "\" SSL/TLS key size is invalid. "
			  << "Expecting a value "
			  << "in the set {2048, 3072, 4096}. Ignoring entry."
			  << std::endl;
	      }
	  }

	int silence = list.at(5).toInt(&o);

	if(!o || silence < 15 || silence > 3600)
	  {
	    listenerOK = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener \""
		      << key.toStdString()
		      << "\" silence value is invalid. Expecting a value "
		      << "in the range [15, 3600]. Ignoring entry."
		      << std::endl;
	  }

	if(hash.contains(list.at(0) + list.at(1)) && listenerOK)
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener ("
		      << list.at(0).toStdString()
		      << ":"
		      << list.at(1).toStdString()
		      << ") is a duplicate. Ignoring entry."
		      << std::endl;
	  }
	else if(listenerOK)
	  m_listeners_properties << settings.value(key).toString();

	if(listenerOK)
	  hash[list.at(0) + list.at(1)] = 0;
      }
    else if(key == "maximum_accumulated_bytes")
      {
	bool o = true;
	int maximumAccumulatedBytes = settings.value(key).toLongLong(&o);

	if(maximumAccumulatedBytes < 1024 || !o)
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "maximum_accumulated_bytes value \""
		      << settings.value(key).toString().toStdString()
		      << "\" is invalid. "
		      << "Expecting a value "
		      << "in the range [1024, "
		      << std::numeric_limits<int>::max()
		      << "]. Ignoring entry."
		      << std::endl;
	  }
	else
	  m_maximum_accumulated_bytes = maximumAccumulatedBytes;
      }
    else if(key == "maximum_number_of_child_processes")
      {
	bool o = true;
	int maximumNumberOfChildProcesses = settings.value(key).toInt(&o);

	if(maximumNumberOfChildProcesses < 0 ||
	   maximumNumberOfChildProcesses > 65535 ||
	   !o)
	  {
	    maximumNumberOfChildProcesses = 128;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "maximum_number_of_child_processes value \""
		      << settings.value(key).toString().toStdString()
		      << "\" is invalid. "
		      << "Expecting a value "
		      << "in the range [1, 65535]. Ignoring entry."
		      << std::endl;
	  }
	else
	  {
	  }
      }
}

void spot_on_lite_daemon::slot_signal_usr1(void)
{
  if(!m_signal_usr1_socket_notifier)
    return;

  m_signal_usr1_socket_notifier->setEnabled(false);

  char a = 0;
  ssize_t rc = ::read(s_signal_usr1_fd[1], &a, sizeof(a));

  Q_UNUSED(rc);
  start();
  m_signal_usr1_socket_notifier->setEnabled(true);
}

void spot_on_lite_daemon::start(void)
{
  m_listeners_properties.clear();
  process_configuration_file(0);
  prepare_listeners();
}

void spot_on_lite_daemon::validate_configuration_file
(const QString &configurationFileName, bool *ok)
{
  m_listeners_properties.clear();
  m_configuration_file_name = configurationFileName;
  process_configuration_file(ok);
}
