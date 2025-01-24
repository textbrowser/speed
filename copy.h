/*
** Copyright (c) 2024, Alexis Megas.
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
**    derived from Speed without specific prior written permission.
**
** SPEED IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** SPEED, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QDir>
#include <QFileInfo>
#include <QtDebug>

class copy
{
 public:
  copy(const QFileInfo &destination,
       const QFileInfo &file_info,
       const quint64 bytes)
  {
    m_bytes = qBound(static_cast<qint64> (1024),
		     static_cast<qint64> (bytes),
		     static_cast<qint64> (131072));
    m_char_array = new char[static_cast<std::size_t> (m_bytes)];
    m_destination = destination.isDir() ?
      QFileInfo(destination.absoluteFilePath() +
		QDir::separator() +
		file_info.fileName()) :
      destination;
    m_file_info = file_info;
  }

  ~copy()
  {
    delete []m_char_array;
  }

  void copy_bytes(void)
  {
    QFile source(m_file_info.absoluteFilePath());

    if(source.open(QIODevice::ReadOnly | QIODevice::Unbuffered) == false)
      {
	qDebug() << QObject::tr("Could not open %1 for reading.").
	  arg(m_file_info.absoluteFilePath());
	return;
      }

    QFile target(m_destination.absoluteFilePath());

    if(target.open(QIODevice::Truncate | QIODevice::WriteOnly) == false)
      {
	qDebug() << QObject::tr("Could not open %1 for writing.").
	  arg(m_destination.absoluteFilePath());
	return;
      }

    auto rc = static_cast<qint64> (0);

    while((rc = source.read(m_char_array, m_bytes)) > 0)
      target.write(m_char_array, rc);
  }

 private:
  QFileInfo m_destination;
  QFileInfo m_file_info;
  char *m_char_array;
  qint64 m_bytes;
};
