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

#include <QCoreApplication>
#include <QDir>
#include <QtConcurrent>
#include <QtDebug>

#include "copy.h"

const static char *version = "2025.01.23";
static QHash<copy *, QFuture<void> > futures;

static void create_future
(const QFileInfo &destination, const QFileInfo &file, const quint64 bytes)
{
  QFuture<void> future;
  auto c = new copy(destination, file, bytes);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  future = QtConcurrent::run(c, &copy::copy_bytes);
#else
  future = QtConcurrent::run(&copy::copy_bytes, c);
#endif
  futures.insert(c, future);
}

int main(int argc, char *argv[])
{
  QFileInfo destination;
  QHash<QString, char> files;
  auto bytes = static_cast<quint64> (4096);
  auto make_destination = false;
  auto overwrite = false;
  auto recursive = false;
  auto speed(QObject::tr("speed [options] origin destination"));

  for(int i = 1; i < argc; i++)
    if(!argv || !argv[i])
      continue;
    else
      {
	if(strcmp(argv[i], "--bytes") == 0)
	  {
	    i += 1;

	    if(argc > i)
	      {
		auto ok = false;

		bytes = QVariant(QString(argv[i])).toULongLong(&ok);

		if(!ok)
		  {
		    qDebug() << QObject::tr("Missing valid --bytes argument.");
		    return EXIT_FAILURE;
		  }
	      }
	    else
	      {
		qDebug() << QObject::tr("Missing --bytes argument.");
		return EXIT_FAILURE;
	      }
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
      }

  for(int i = 1; i < argc; i++)
    if(!argv || !argv[i])
      continue;
    else
      {
	if(strcmp(argv[i], "--bytes") == 0)
	  {
	    i += 1;
	    continue;
	  }
	else if(strcmp(argv[i], "--make-destination") == 0 ||
		strcmp(argv[i], "--overwrite") == 0 ||
		strcmp(argv[i], "--recursive") == 0 ||
		strcmp(argv[i], "--version") == 0)
	  continue;
	else if(argc - 1 == i)
	  destination = QFileInfo(argv[i]);
	else
	  {
	    QFileInfo const file(argv[i]);

	    if(file.isReadable())
	      {
		if(file.isDir())
		  {
		    if(!recursive)
		      {
			qDebug() << QObject::tr
			  ("Directory %1 specified without "
			   "--recursive option.").arg(file.absoluteFilePath());
			continue;
		      }

		    files.insert(file.absoluteFilePath(), 0);
		  }
		else
		  files.insert(file.absoluteFilePath(), 0);
	      }
	    else
	      qDebug() << QObject::tr("The file %1 is not readable.").
		arg(file.absoluteFilePath());
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

  QHashIterator<QString, char> it_a(files);

  while(it_a.hasNext())
    {
      it_a.next();

      QFileInfo const file(it_a.key());

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

      if(destination.isDir() && file.isDir() == false)
	{
	  QFileInfo const d
	    (destination.absoluteFilePath() +
	     QDir::separator() +
	     file.fileName());

	  if(d.canonicalFilePath() == file.canonicalFilePath())
	    {
	      qDebug() << QObject::tr
		("Cannot write %1 onto itself. Skipping.").
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

      if(file.isDir() && file.isReadable())
	{
	  QDirIterator it
	    (file.absoluteFilePath(), QDirIterator::Subdirectories);
	  auto directory(file.absoluteFilePath());

	  if(directory.endsWith(QDir::separator()))
	    // QDir::separator() contains a single character.

	    directory = directory.mid(0, directory.length() - 1);

	  directory = QFileInfo(directory).fileName();

	  while(it.hasNext())
	    {
	      QFileInfo const file(it.next());

	      if(file.isDir())
		{
		  if(file.isHidden() == false)
		    QDir().mkpath
		      (destination.absoluteFilePath() +
		       QDir::separator() +
		       directory +
		       QDir::separator() +
		       file.fileName());
		}
	      else if(file.isFile())
		{
		  if(file.isReadable())
		    {
		      auto d = destination.absoluteFilePath() +
			QDir::separator();

		      d += file.absoluteFilePath().mid
			(file.absoluteFilePath().indexOf(directory));

		      if(QFileInfo(d).exists() && overwrite == false)
			qDebug() << QObject::tr
			  ("The file %1 already exists. Skipping.").arg(d);
		      else
			create_future(QFileInfo(d), file, bytes);
		    }
		  else
		    qDebug() << QObject::tr
		      ("The file %1 is not readable.").
		      arg(file.absoluteFilePath());
		}
	    }

	  continue;
	}

      if(file.isReadable() == false)
	{
	  qDebug() << QObject::tr("Cannot read %1. Skipping.").
	    arg(file.fileName());
	  continue;
	}

      create_future(destination, file, bytes);
    }

  QMutableHashIterator<copy *, QFuture<void> > it_b(futures);

  while(it_b.hasNext())
    {
      it_b.next();

      auto const future(it_b.value());

      if(future.isFinished())
	{
	  delete it_b.key();
	  it_b.remove();
	}
      else
	it_b.toFront();
    }

  return EXIT_SUCCESS;
}
