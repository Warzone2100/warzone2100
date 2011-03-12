/*
	This file is part of Warzone 2100.
	Copyright (C) 2011  Warzone 2100 Project

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

// todo use utf-8 when physfs 2.0 is supported on windows

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
	QDateTime	lastMod;	// PHYSFS_getLastModTime

public:
	PhysicsFileSystem(QString filename) { fp = NULL; setFileName(filename); }
	virtual ~PhysicsFileSystem() { if (fp) PHYSFS_close(fp); }
	bool atEnd() const { return PHYSFS_eof(fp) != 0; }
	//virtual Iterator *beginEntryList(QDir::Filters filters, const QStringList & filterNames) { return NULL; }
	virtual bool caseSensitive() const { return true; }
	virtual bool close() { if (fp) { int retval = PHYSFS_close(fp); fp = NULL; return retval != 0; } else return true; }
	//virtual bool copy(const QString & newName) { return false; }
	virtual QStringList entryList(QDir::Filters filters, const QStringList & filterNames) const { return QStringList(); } 	// dummy, TODO
	QFile::FileError error() const { return QFile::UnspecifiedError; }
	QString errorString() const { return QString(PHYSFS_getLastError()); }
	virtual bool extension(Extension extension, const ExtensionOption * option = 0, ExtensionReturn * output = 0) { return extension == QAbstractFileEngine::AtEndExtension; }
	virtual FileFlags fileFlags(FileFlags type = FileInfoAll) const { return flags | type; }
	virtual QDateTime fileTime(FileTime time) const { if (time == QAbstractFileEngine::ModificationTime) return lastMod; else return QDateTime(); }
	virtual bool flush() { return PHYSFS_flush(fp) != 0; }
	//virtual int handle() const { return 0; }
	virtual bool isRelativePath() const { return true; }	// in physfs, all paths are relative
	virtual bool isSequential() const { return true; }
	virtual bool link(const QString & newName) { return false; }
	//uchar *map(qint64 offset, qint64 size, QFile::MemoryMapFlags flags) { return NULL; }
	virtual bool mkdir(const QString & dirName, bool createParentDirectories) const { Q_UNUSED(createParentDirectories); return PHYSFS_mkdir(dirName.toAscii().constData()) != 0; }
	virtual QString owner(FileOwner owner) const { qWarning("owner() not supported"); return QString(); }
	virtual uint ownerId(FileOwner owner) const { qWarning("ownerId() not supported"); return -2; }
	virtual qint64 pos() const { return PHYSFS_tell(fp); }
	virtual qint64 read(char *data, qint64 maxlen) { return PHYSFS_read(fp, data, 1, maxlen); }
	//virtual qint64 readLine(char * data, qint64 maxlen) { return 0; }
	virtual bool remove() { return PHYSFS_delete(name.toAscii().constData()) != 0; }
	//virtual bool rename(const QString & newName) { return false; }
	virtual bool rmdir(const QString & dirName, bool recurseParentDirectories) const { Q_UNUSED(recurseParentDirectories); return PHYSFS_delete(name.toAscii().constData()) != 0; }
	virtual bool seek(qint64 offset) { return PHYSFS_seek(fp, offset) != 0; }
	virtual bool setPermissions(uint perms) { return false; }
	virtual bool setSize(qint64 size) { return false; }	// oops... no mapping for physfs here!
	virtual bool supportsExtension(Extension extension) const { return extension == QAbstractFileEngine::AtEndExtension; }
	//bool unmap(uchar * address) { return NULL; }
	virtual qint64 write(const char * data, qint64 len) { return PHYSFS_write(fp, data, len, 1); }

	virtual qint64 size() const
	{
		if (!fp)
		{
			PHYSFS_file *tmp;
			tmp = PHYSFS_openRead(name.toAscii().constData());
			if (!tmp)
			{
				qWarning("Failed to open %s for size info: %s", name.toAscii().constData(), PHYSFS_getLastError());
				return -1;
			}
			int retval = PHYSFS_fileLength(tmp);
			PHYSFS_close(tmp);
			return retval;
		}
		return PHYSFS_fileLength(fp);
	}

	virtual void setFileName(const QString & file)
	{
		name = file;
		lastMod = QDateTime::fromTime_t(PHYSFS_getLastModTime(name.toAscii().constData()));
		if (PHYSFS_exists(name.toAscii().constData()) != 0) flags = QAbstractFileEngine::ExistsFlag;
		if (PHYSFS_isDirectory(name.toAscii().constData()) != 0) flags |= QAbstractFileEngine::DirectoryType;
		if (PHYSFS_isSymbolicLink(name.toAscii().constData()) != 0) flags |= QAbstractFileEngine::LinkType;
		if (!(flags & QAbstractFileEngine::DirectoryType || flags & QAbstractFileEngine::LinkType)) flags |= QAbstractFileEngine::FileType;
	}

	virtual bool open(QIODevice::OpenMode mode)
	{
		close();
		if (mode & QIODevice::ReadOnly)
		{
			flags = QAbstractFileEngine::ReadOwnerPerm | QAbstractFileEngine::ReadUserPerm | QAbstractFileEngine::FileType;
			fp = PHYSFS_openRead(name.toAscii().constData());
		}
		else if (mode & QIODevice::WriteOnly)
		{
			flags = QAbstractFileEngine::WriteOwnerPerm | QAbstractFileEngine::WriteUserPerm | QAbstractFileEngine::FileType;
			fp = PHYSFS_openWrite(name.toAscii().constData());	// will truncate
		}
		else if (mode & QIODevice::Append)
		{
			flags = QAbstractFileEngine::WriteOwnerPerm | QAbstractFileEngine::WriteUserPerm | QAbstractFileEngine::FileType;
			fp = PHYSFS_openAppend(name.toAscii().constData());
		}
		else
		{
			qWarning("Bad file open mode: %d", (int)mode);
		}
		if (fp) flags |= QAbstractFileEngine::ExistsFlag;
		else qWarning("Failed to open %s: %s", name.toAscii().constData(), PHYSFS_getLastError());
		return fp != NULL;
	}

	virtual QString fileName(FileName file = DefaultName) const
	{
		switch (file)
		{
		case QAbstractFileEngine::DefaultName:
			return name;
		case QAbstractFileEngine::CanonicalName:
		case QAbstractFileEngine::AbsoluteName:
			if (PHYSFS_exists(name.toAscii().constData()))
			{
				return QString(PHYSFS_getRealDir(name.toAscii().constData())) + PHYSFS_getDirSeparator() + name;
			}
			else
			{
				return QString(PHYSFS_getWriteDir()) + PHYSFS_getDirSeparator() + name;
			}
		default:
			qWarning("Unsupported path lookup type (%d)", (int)file);
			return QString(PHYSFS_getRealDir(name.toAscii().constData()));
		}
	}

};

#endif
