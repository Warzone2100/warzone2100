/*
	This file is part of Warzone 2100.
	Copyright (C) 2013  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
#ifndef QTPHYSFS_H
#define QTPHYSFS_H

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QAbstractFileEngine>
#include <physfs.h>

class PhysicsFileSystem : public QAbstractFileEngine
{
private:
	PHYSFS_file	*fp;
	FileFlags	flags;
	QString		name;
	QDateTime	lastMod;

	void realSetFileName(const QString &file)
	{
		name = file;
		if (!PHYSFS_exists(name.toUtf8().constData()))
		{
			return;
		}
		// Show potential until actually opened
		flags = QAbstractFileEngine::ExistsFlag | QAbstractFileEngine::ReadOtherPerm | QAbstractFileEngine::WriteOwnerPerm \
		        | QAbstractFileEngine::ReadOwnerPerm | QAbstractFileEngine::ReadUserPerm | QAbstractFileEngine::WriteUserPerm;
		lastMod = QDateTime::fromTime_t(PHYSFS_getLastModTime(name.toUtf8().constData()));
		if (PHYSFS_isDirectory(name.toUtf8().constData()))
		{
			flags |= QAbstractFileEngine::DirectoryType;
		}
		if (PHYSFS_isSymbolicLink(name.toUtf8().constData()))
		{
			flags |= QAbstractFileEngine::LinkType;
		}
		if (!(flags & QAbstractFileEngine::DirectoryType || flags & QAbstractFileEngine::LinkType))
		{
			flags |= QAbstractFileEngine::FileType;
		}
	}


public:
	PhysicsFileSystem(QString filename) : fp(NULL), flags(0), name(filename)
	{
		realSetFileName(filename);
	}
	virtual ~PhysicsFileSystem()
	{
		if (fp)
		{
			PHYSFS_close(fp);
		}
	}
	bool atEnd() const
	{
		return PHYSFS_eof(fp) != 0;
	}
	virtual bool caseSensitive() const
	{
		return true;
	}
	virtual bool close()
	{
		if (fp)
		{
			int retval = PHYSFS_close(fp);
			fp = NULL;
			return retval != 0;
		}
		else
		{
			return true;
		}
	}
	QFile::FileError error() const
	{
		return QFile::UnspecifiedError;
	}
	QString errorString() const
	{
		return QString(PHYSFS_getLastError());
	}
	virtual bool extension(Extension extension, const ExtensionOption *option = 0, ExtensionReturn *output = 0)
	{
		return extension == QAbstractFileEngine::AtEndExtension;
	}
	virtual FileFlags fileFlags(FileFlags type = FileInfoAll) const
	{
		return type & flags;
	}
	virtual QDateTime fileTime(FileTime time) const
	{
		if (time == QAbstractFileEngine::ModificationTime)
		{
			return lastMod;
		}
		else
		{
			return QDateTime();
		}
	}
	virtual bool flush()
	{
		return PHYSFS_flush(fp) != 0;
	}
	virtual bool isRelativePath() const
	{
		return true;    // in physfs, all paths are relative
	}
	virtual bool isSequential() const
	{
		return true;
	}
	virtual bool mkdir(const QString &dirName, bool createParentDirectories) const
	{
		Q_UNUSED(createParentDirectories);
		return PHYSFS_mkdir(dirName.toUtf8().constData()) != 0;
	}
	virtual qint64 pos() const
	{
		return PHYSFS_tell(fp);
	}
	virtual qint64 read(char *data, qint64 maxlen)
	{
		return PHYSFS_read(fp, data, 1, maxlen);
	}
	virtual bool remove()
	{
		return PHYSFS_delete(name.toUtf8().constData()) != 0;
	}
	virtual bool rmdir(const QString &dirName, bool recurseParentDirectories) const
	{
		Q_UNUSED(recurseParentDirectories);
		return PHYSFS_delete(name.toUtf8().constData()) != 0;
	}
	virtual bool seek(qint64 offset)
	{
		return PHYSFS_seek(fp, offset) != 0;
	}
	virtual bool supportsExtension(Extension extension) const
	{
		return extension == QAbstractFileEngine::AtEndExtension;
	}
	virtual qint64 write(const char *data, qint64 len)
	{
		return PHYSFS_write(fp, data, 1, len);
	}

	virtual qint64 size() const
	{
		if (!fp)
		{
			if (!PHYSFS_exists(name.toUtf8().constData()))
			{
				return 0;
			}
			PHYSFS_file *tmp;
			tmp = PHYSFS_openRead(name.toUtf8().constData());
			if (!tmp)
			{
				qWarning("Failed to open %s for size info: %s", name.toUtf8().constData(), PHYSFS_getLastError());
				return -1;
			}
			int retval = PHYSFS_fileLength(tmp);
			PHYSFS_close(tmp);
			return retval;
		}
		return PHYSFS_fileLength(fp);
	}

	virtual void setFileName(const QString &file)
	{
		realSetFileName(file);
	}

	virtual bool open(QIODevice::OpenMode mode)
	{
		close();
		if (mode & QIODevice::WriteOnly)
		{
			flags = QAbstractFileEngine::WriteOwnerPerm | QAbstractFileEngine::WriteUserPerm | QAbstractFileEngine::FileType;
			fp = PHYSFS_openWrite(name.toUtf8().constData());	// will truncate
			if (!fp)
			{
				qWarning("Failed to create %s: %s", name.toUtf8().constData(), PHYSFS_getLastError());
			}
		}
		else if (mode & QIODevice::ReadOnly)
		{
			flags = QAbstractFileEngine::ReadOwnerPerm | QAbstractFileEngine::ReadUserPerm | QAbstractFileEngine::FileType | QAbstractFileEngine::ReadOtherPerm;
			fp = PHYSFS_openRead(name.toUtf8().constData());
			// since open is sometimes used to check for existence, do not complain here
		}
		else if (mode & QIODevice::Append)
		{
			flags = QAbstractFileEngine::WriteOwnerPerm | QAbstractFileEngine::WriteUserPerm | QAbstractFileEngine::FileType;
			fp = PHYSFS_openAppend(name.toUtf8().constData());
			if (!fp)
			{
				qWarning("Failed to append open %s: %s", name.toUtf8().constData(), PHYSFS_getLastError());
			}
		}
		else
		{
			qWarning("Bad file open mode: %d", (int)mode);
		}
		if (fp)
		{
			flags |= QAbstractFileEngine::ExistsFlag;
		}
		return fp != NULL;
	}

	virtual QString fileName(FileName file = DefaultName) const
	{
		if (file == QAbstractFileEngine::AbsolutePathName)
		{
			return PHYSFS_getWriteDir();    // hack for QSettings
		}
		return "wz::" + name;
	}

};

#endif
