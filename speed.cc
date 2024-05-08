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
**    derived from Glitch without specific prior written permission.
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

#include <QCoreApplication>
#include <QDir>
#include <QtConcurrent>
#include <QtDebug>

#include "copy.h"

int main(int argc, char *argv[])
{
  QFileInfo destination;
  QFileInfoList files;
  QHash<QString, char> contains;
  auto bytes = static_cast<quint64> (4096);

  for(int i = 0; i < argc; i++)
    if(argv && argv[i])
      {
	if(argc - 1 == i)
	  destination = QFileInfo(argv[i]);
	else if(strcmp(argv[i], "--bytes") == 0)
	  {
	    i += 1;

	    if(argc > i)
	      bytes = QVariant(argv[i]).toULongLong();
	  }
	else if(i > 0)
	  {
	    QDir directory(argv[i]);
	    auto list
	      (directory.
	       entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::Readable));

	    if(list.isEmpty())
	      {
		if(!contains.contains(argv[i]))
		  {
		    contains[argv[i]] = 0;
		    files << QFileInfo(argv[i]);
		  }
	      }
	    else
	      foreach(auto const &file, list)
		if(!contains.contains(file.canonicalFilePath()))
		  {
		    if(file.isDir())
		      qDebug() << QObject::tr
			("The file %1 is a directory. Skipping.").
			arg(file.fileName());
		    else
		      {
			contains[file.canonicalFilePath()] = 0;
			files << file;
		      }
		  }
	  }
      }

  if(!destination.isWritable())
    {
      qDebug() << QObject::tr("Please specify a writable destination.");
      return EXIT_FAILURE;
    }
  else if(files.isEmpty())
    {
      qDebug() << QObject::tr("Please provide at least one file.");
      return EXIT_FAILURE;
    }

  QList<QFuture<void> > futures;

  for(int i = 0; i < files.size(); i++)
    {
      const auto &file(files.at(i));

      if(destination.isDir())
	{
	  if(destination.canonicalFilePath() == file.canonicalPath())
	    {
	      qDebug() << QObject::tr("The file %1 already exists. Skipping.").
		arg(file.fileName());
	      continue;
	    }
	}

      if(destination.canonicalFilePath() == file.canonicalFilePath())
	{
	  qDebug() << QObject::tr("Cannot copy %1 onto %2. Skipping.").
	    arg(file.fileName()).
	    arg(destination.fileName());
	  continue;
	}

      if(file.isReadable() == false)
	{
	  qDebug() << QObject::tr("Cannot read %1. Skipping.").
	    arg(file.fileName());
	  continue;
	}

      QFuture<void> future;
      auto c = new copy(destination, file, bytes);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      future = QtConcurrent::run(c, &copy::copy_bytes);
#else
      future = QtConcurrent::run(&copy::copy_bytes, c);
#endif
      futures << future;
    }

  for(int i = 0; i < futures.size(); i++)
    if(!futures[i].isFinished())
      futures[i].waitForFinished();

  return EXIT_SUCCESS;
}
