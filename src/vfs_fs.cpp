#include <unistd.h>
#include <sys/stat.h>
#include "vfs_api.h"
#include "vfs_fs.h"

using namespace fs;

VFSFS fs::VFS;

VFSFS::VFSFS() : FS(FSImplPtr(new VFSImpl()))
{
    _impl->mountpoint("");
}

VFSFS::~VFSFS()
{
}