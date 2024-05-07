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
#include <QtDebug>

#include "copy.h"

int main(int argc, char *argv[])
{
  QFileInfo destination;
  QFileInfoList files;
  QHash<QString, char> contains;
  auto bytes = static_cast<quint64> (4096);
  auto tasks = static_cast<quint64> (8);

  for(int i = 0; i < argc; i++)
    if(argv && argv[i])
      {
	if(argc - 1 == i)
	  destination = QFileInfo(argv[i]);
	else if(i > 0)
	  {
	    QDir directory;
	    auto list(directory.entryInfoList(QStringList() << argv[i]));

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
		    contains[file.canonicalFilePath()] = 0;
		    files << list;
		  }
	  }
	else if(strcmp(argv[i], "--bytes") == 0)
	  {
	    i += 1;

	    if(argc > i)
	      bytes = QVariant(argv[i]).toULongLong();
	  }
	else if(strcmp(argv[i], "--tasks") == 0)
	  {
	    i += 1;

	    if(argc > i)
	      tasks = QVariant(argv[i]).toULongLong();
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

  QMutableListIterator<QFileInfo> it(files);

  while(it.hasNext())
    {
      it.next();

      if(destination.isDir())
	{
	  if(destination.canonicalFilePath() == it.value().canonicalPath())
	    {
	      qDebug() << QObject::tr("File %1 already exists. Skipping.").
		arg(it.value().fileName());
	      it.remove();
	      continue;
	    }
	}

      if(destination.canonicalFilePath() == it.value().canonicalFilePath())
	{
	  qDebug() << QObject::tr("Cannot copy %1 onto %2. Skipping.").
	    arg(it.value().fileName()).
	    arg(destination.fileName());
	  it.remove();
	  continue;
	}

      if(it.value().isReadable() == false)
	{
	  qDebug() << QObject::tr("Cannot read %1. Skipping.").
	    arg(it.value().fileName());
	  it.remove();
	}
    }

  if(files.isEmpty())
    return EXIT_FAILURE;

  bytes = qMax(bytes, static_cast<quint64> (1024));
  tasks = qMax(static_cast<quint64> (1), tasks);

  QCoreApplication application(argc, argv);

  for(int i = 0; i < files.size(); i++)
    {
      copy c(destination, files.at(i), bytes, tasks);
    }

  auto rc = application.exec();

  return static_cast<int> (rc);
}
