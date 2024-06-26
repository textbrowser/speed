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

#include <QCoreApplication>
#include <QDir>
#include <QtConcurrent>
#include <QtDebug>

#include "copy.h"

const static char *version = "2024.05.08";

int main(int argc, char *argv[])
{
  QFileInfo destination;
  QFileInfoList files;
  QHash<QString, char> contains;
  auto bytes = static_cast<quint64> (4096);
  auto make_destination = false;
  auto overwrite = false;
  auto recursive = false;

  for(int i = 0; i < argc; i++)
    if(Q_LIKELY(argv && argv[i]))
      {
	if(argc - 1 == i)
	  {
	    if(strcmp(argv[i], "--make-destination") == 0)
	      make_destination = true;
	    else if(strcmp(argv[i], "--overwrite") == 0)
	      overwrite = true;
	    else if(strcmp(argv[i], "--recursive") == 0)
	      recursive = true;
	    else if(strcmp(argv[i], "--version") == 0)
	      {
		qDebug() << "speed: " << version;
		return EXIT_SUCCESS;
	      }
	    else
	      destination = QFileInfo(argv[i]);
	  }
	else if(strcmp(argv[i], "--bytes") == 0)
	  {
	    i += 1;

	    if(argc > i)
	      bytes = QVariant(QString(argv[i])).toULongLong();
	  }
	else if(strcmp(argv[i], "--make-destination") == 0)
	  make_destination = true;
	else if(strcmp(argv[i], "--overwrite") == 0)
	  overwrite = true;
	else if(strcmp(argv[i], "--recursive") == 0)
	  recursive = true;
	else if(strcmp(argv[i], "--version") == 0)
	  {
	    qDebug() << "speed: " << version;
	    return EXIT_SUCCESS;
	  }
	else if(i > 0)
	  {
	    QDir directory(argv[i]);
	    auto list
	      (directory.
	       entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::Readable));

	    if(list.isEmpty())
	      {
		if(contains.contains(argv[i]) == false)
		  {
		    contains[argv[i]] = 0;

		    auto file(QFileInfo(argv[i]));

		    if(file.isDir() && recursive == false)
		      qDebug() << QObject::tr
			("The file %1 is a directory. Skipping.").
			arg(file.fileName());
		    else
		      files << file;
		  }
	      }
	    else
	      foreach(auto const &file, list)
		if(contains.contains(file.canonicalFilePath()) == false)
		  {
		    contains[file.canonicalFilePath()] = 0;

		    if(file.isDir() && recursive == false)
		      qDebug() << QObject::tr
			("The file %1 is a directory. Skipping.").
			arg(file.fileName());
		    else
		      files << file;
		  }
	  }
      }

  if(destination.isWritable() == false && make_destination == false)
    {
      qDebug() << QObject::tr("Please specify a writable destination.");
      return EXIT_FAILURE;
    }

  if(files.isEmpty())
    {
      qDebug() << QObject::tr("Please provide at least one file.");
      return EXIT_FAILURE;
    }

  if(make_destination)
    {
      QDir directory;

      Q_UNUSED(directory.mkpath(destination.absoluteFilePath()));
    }

  QList<QFuture<void> > futures;
  QList<copy *> copies;

  for(int i = 0; i < files.size(); i++)
    {
      const auto &file(files.at(i));

      if(destination.canonicalFilePath() == file.canonicalFilePath())
	{
	  qDebug() << QObject::tr("Cannot write %1 onto itself. Skipping.").
	    arg(destination.canonicalFilePath());
	  continue;
	}

      if(destination.exists() &&
	 destination.isDir() == false &&
	 overwrite == false)
	{
	  qDebug() << QObject::tr("The file %1 already exists. Skipping.").
	    arg(destination.absoluteFilePath());
	  continue;
	}

      if(destination.isDir())
	{
	  QFileInfo d
	    (destination.absoluteFilePath() +
	     QDir::separator() +
	     file.fileName());

	  if(d.canonicalFilePath() == file.canonicalFilePath())
	    {
	      qDebug() << QObject::tr("Cannot write %1 onto itself. Skipping.").
		arg(d.canonicalFilePath());
	      continue;
	    }

	  if(d.exists() && overwrite == false)
	    {
	      qDebug() << QObject::tr("The file %1 already exists. Skipping.").
		arg(d.absoluteFilePath());
	      continue;
	    }
	}

      if(file.isReadable() == false)
	{
	  qDebug() << QObject::tr("Cannot read %1. Skipping.").
	    arg(file.fileName());
	  continue;
	}

      QFuture<void> future;
      auto c = new copy(destination, file, bytes);

      copies << c;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      future = QtConcurrent::run(c, &copy::copy_bytes);
#else
      future = QtConcurrent::run(&copy::copy_bytes, c);
#endif
      futures << future;
    }

  for(int i = 0; i < copies.size(); i++)
    {
      if(futures[i].isFinished() == false)
	futures[i].waitForFinished();

      delete copies.at(i);
    }

  return EXIT_SUCCESS;
}
