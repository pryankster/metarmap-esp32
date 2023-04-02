#ifndef _H_VFS_FS_
#define _H_VFS_FS_

#include <Arduino.h>
#include <FS.h>

namespace fs {

class VFSFS : public FS 
{
public:
    VFSFS();
    ~VFSFS();
};

extern fs::VFSFS VFS;

};

#endif // _H_VFS_FS_