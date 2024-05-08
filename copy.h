/*
** Copyright (c) 2024, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met
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
#include <QObject>
#include <QtDebug>

class copy: public QObject
{
  Q_OBJECT

 public:
  copy(const QFileInfo &destination,
       const QFileInfo &file_info,
       const quint64 bytes):QObject()
  {
    m_bytes = qBound(static_cast<qint64> (1024),
		     static_cast<qint64> (bytes),
		     static_cast<qint64> (131072));
    m_destination = destination.isDir() ?
      QFileInfo(destination.absoluteFilePath() +
		QDir::separator() +
		file_info.fileName()) :
      destination;
    m_file_info = file_info;
  }

  ~copy()
  {
  }

  void copy_bytes(void)
  {
    QFile destination(m_destination.absoluteFilePath());

    if(!destination.open(QIODevice::WriteOnly))
      return;

    QFile file(m_file_info.absoluteFilePath());

    if(!file.open(QIODevice::ReadOnly))
      return;

    auto bytes = new char[m_bytes];
    auto rc = static_cast<qint64> (0);

    while((rc = file.read(bytes, m_bytes)) > 0)
      destination.write(bytes, rc);

    delete []bytes;
    delete this;
  }

 private:
  QFileInfo m_destination;
  QFileInfo m_file_info;
  qint64 m_bytes;
};
