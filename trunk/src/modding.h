#define PHYSFS_APPEND 1
#define PHYSFS_PREPEND 0

void addSubdirs( const char * basedir, const char * subdir, const BOOL appendToPath, char * checkList[] );
void removeSubdirs( const char * basedir, const char * subdir, char * checkList[] );
void printSearchPath( void );
