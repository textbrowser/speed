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

const static char *version = "2024.07.10";

int main(int argc, char *argv[])
{
  QFileInfo destination;
  QFileInfoList files;
  QHash<QString, char> contains;
  auto bytes = static_cast<quint64> (4096);
  auto make_destination = false;
  auto overwrite = false;
  auto speed(QObject::tr("speed [options] origin destination"));

  for(int i = 1; i < argc; i++)
    if(Q_LIKELY(argv && argv[i]))
      {
	if(strcmp(argv[i], "--bytes") == 0)
	  {
	    i += 1;

	    if(argc > i)
	      bytes = QVariant(QString(argv[i])).toULongLong();
	  }
	else if(strcmp(argv[i], "--make-destination") == 0)
	  make_destination = true;
	else if(strcmp(argv[i], "--overwrite") == 0)
	  overwrite = true;
	else if(strcmp(argv[i], "--version") == 0)
	  {
	    qDebug() << "speed: " << version;
	    return EXIT_SUCCESS;
	  }
	else
	  {
	    if(argc - 1 == i)
	      destination = QFileInfo(argv[i]);
	    else
	      {
		auto file(QFileInfo(argv[i]));

		if(file.isReadable())
		  {
		    if(file.isDir())
		      {
			QDirIterator it
			  (file.absoluteFilePath(),
			   QDirIterator::Subdirectories);

			while(it.hasNext())
			  {
			    QFileInfo file(it.next());

			    if(file.isFile())
			      {
				if(file.isReadable())
				  files << file;
				else
				  qDebug() << QObject::tr
				    ("The file %1 is not readable.").
				    arg(file.absoluteFilePath());
			      }
			  }
		      }
		    else
		      files << file;
		  }
		else
		  qDebug() << QObject::tr("The file %1 is not readable.").
		    arg(file.absoluteFilePath());
	      }
	  }
      }

  if(destination.exists() &&
     destination.isWritable() == false &&
     make_destination == false)
    {
      qDebug() << QObject::tr("Please specify a writable destination.");
      qDebug() << speed;
      return EXIT_FAILURE;
    }

  if(destination.isDir() == false &&
     files.size() > 1 &&
     make_destination == false)
    {
      qDebug() << QObject::tr("Please specify a writable directory.");
      qDebug() << speed;
      return EXIT_FAILURE;
    }

  if(destination.path().isEmpty())
    {
      qDebug() << QObject::tr("Please specify a writable destination.");
      qDebug() << speed;
      return EXIT_FAILURE;
    }

  if(files.isEmpty())
    {
      qDebug() << QObject::tr("Please provide at least one file.");
      qDebug() << speed;
      return EXIT_FAILURE;
    }

  if(make_destination)
    {
      if(destination.path().isEmpty())
	{
	  qDebug() << speed;
	  return EXIT_FAILURE;
	}
      else
	{
	  QDir directory;

	  if(!directory.mkpath(destination.absoluteFilePath()))
	    {
	      qDebug() << QObject::tr("Cannot create the path %1.").
		arg(destination.absoluteFilePath());
	      return EXIT_FAILURE;
	    }
	}
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
