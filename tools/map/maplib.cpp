#include "maplib.h"

void physfs_init(const char* binpath)
{
    PHYSFS_init(binpath);

    // Add cwd
    PHYSFS_setWriteDir(".");
    PHYSFS_mount(".", NULL, 1);
}

void physfs_shutdown()
{
    PHYSFS_deinit();
}

char* physfs_addmappath(char *path)
{
    char *p_path, *p_result;
    char *result;
    int length;

    result = (char *)malloc(PATH_MAX * sizeof(char));
    result[0] = '\0';
    p_result = result;
    
    length = strlen(path);
    if (length >= 3 && strcmp(&path[length-3], ".wz")==0)
    {
        PHYSFS_mount(path, NULL, 1);
            
        strcat(p_result, "multiplay/maps/");
        p_result += strlen("multiplay/maps/");        
        
        p_path = strrchr(path, PHYSFS_getDirSeparator()[0]);
        if (p_path)
        {
            // Remove the path
            p_path++;
            strcpy(p_result, p_path);

			path[strlen(path) - strlen(result) - 1] = '\0';
			PHYSFS_mount(path, NULL, 1);
        }
        else
        {
            strcpy(p_result, path);
        }

        // remove ".wz" from end
        length = strlen(result);
        result[length-3] = '\0';
    }
    else
    {
        if (PHYSFS_exists("multiplay/maps"))
        {
            strcat(p_result, "multiplay/maps/");
            p_result += strlen("multiplay/maps/");
        }

        p_path = strrchr(path, PHYSFS_getDirSeparator()[0]);
        if (p_path)
        {
            // Remove the path
            p_path++;
            strcpy(p_result, p_path);
        }
        else
        {
            strcpy(p_result, path);
        }        
    }

    return result;
}

void physfs_printSearchPath()
{
    char ** i, ** searchPath;

    debug(0, "%s", "Search paths:");
    searchPath = PHYSFS_getSearchPath();
    for (i = searchPath; *i != NULL; i++)
    {
        debug(0, "    [%s]", *i);
    }
    PHYSFS_freeList(searchPath);
}
