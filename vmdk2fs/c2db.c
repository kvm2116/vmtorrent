/* Support Large File */
#define _FILE_OFFSET_BITS 64
#define __USE_LARGEFILE64


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <linux/fiemap.h>
#include <linux/fs.h>
#include <sqlite3.h>
#include <sys/ioctl.h>
#include <unistd.h>

void choppy( char *s )
{
    s[strcspn ( s, "\n" )] = '\0';
}


int main (int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr,"usage: %s <file list>\n", argv[0]);
        return -1;
    }

    sqlite3 *db = NULL;
    int rc;
    char *zErr;
    char* sql;



    if (argc == 3)
    {
        sqlite3_initialize();
        rc = sqlite3_open(argv[2], &db);

        if (rc)
        {
            fprintf(stderr, "Can't open database %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            exit(1);
        }

        sql = "CREATE TABLE IF NOT EXISTS files (start INTEGER NOT NULL, end INTEGER NOT NULL,"
                                   "name TEXT NOT NULL, length INTEGER NOT NULL,"
                                   "PRIMARY KEY (start ASC))";

        rc = sqlite3_exec(db, sql, NULL, NULL, &zErr);

        if (rc != SQLITE_OK) {
            if (zErr != NULL) {
                    fprintf(stderr, "SQL error: %s\n", zErr);
                    sqlite3_free(zErr);
            }
        }

    }

    FILE *listfile;

    listfile = fopen(argv[1], "r");
    if (listfile == NULL)
    {
        fprintf(stderr, "can't open list file!\n");
        return -1;
    }


    char filename[2048];
    int fd;

    int i;
    struct fiemap *fiemap;
    int extents_size;
    long long int filelength;
    sqlite3_stmt *stmt = NULL;

    if ((fiemap = (struct fiemap*)malloc(sizeof(struct fiemap))) == NULL) {
        fprintf(stderr, "Out of memory allocating fiemap\n");
        return -1;
    }



    if (db)
    {
        sqlite3_exec(db,"BEGIN",NULL, NULL, &zErr);
        sql = "INSERT INTO files VALUES (:start,:end,:name,:length)";
        rc = sqlite3_prepare_v2(db, sql, -1 ,&stmt, NULL);
        if (rc != SQLITE_OK)
            exit (-1);
    }

	struct stat st;

    while (fgets(filename, 2048, listfile)!= NULL)
    {
        choppy(filename);

        if ((fd = open(filename, O_RDONLY)) < 0) {
            //fprintf(stderr, "error opening: %s\n",filename);
            perror(filename);
            continue;
        }


        memset(fiemap, 0, sizeof(struct fiemap));

        fiemap->fm_start = 0;
        fiemap->fm_length = ~0;
        fiemap->fm_flags = 0;
        fiemap->fm_extent_count = 0;
        fiemap->fm_mapped_extents = 0;

        if (ioctl(fd, FS_IOC_FIEMAP, fiemap) < 0) {
            fprintf(stderr, "fiemap not supported on: %s\n",filename);
            continue;
        }

        extents_size = sizeof(struct fiemap_extent) * (fiemap->fm_mapped_extents);
        if ((fiemap = (struct fiemap*)realloc(fiemap, sizeof(struct fiemap) + extents_size)) == NULL) {
            fprintf(stderr, "Out of memory allocating fiemap\n");
            return -1;
        }

        memset(fiemap->fm_extents, 0, extents_size);
        fiemap->fm_extent_count = fiemap->fm_mapped_extents;
        fiemap->fm_mapped_extents = 0;

        if (ioctl(fd, FS_IOC_FIEMAP, fiemap) < 0) {
            fprintf(stderr, "fiemap not supported on: %s\n",filename);
            continue;
        }

        //filelength = fiemap->fm_extents[fiemap->fm_mapped_extents-1].fe_logical + fiemap->fm_extents[fiemap->fm_mapped_extents-1].fe_length;
		
		fstat(fd, &st);
		filelength = st.st_size;

        for (i=0; i<fiemap->fm_mapped_extents;i++) {
            if (db)
            {
                sqlite3_bind_int64(stmt,sqlite3_bind_parameter_index(stmt, ":start"), fiemap->fm_extents[i].fe_physical);
                sqlite3_bind_int64(stmt,sqlite3_bind_parameter_index(stmt, ":end"), fiemap->fm_extents[i].fe_physical + fiemap->fm_extents[i].fe_length);
                sqlite3_bind_text(stmt, sqlite3_bind_parameter_index(stmt, ":name"), filename, strlen(filename), SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt,sqlite3_bind_parameter_index(stmt, ":length"), filelength);

                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
            else
            {
                printf("%-20.20lld\t%-20.20lld\t%s\t%-20.20lld\t%-20.20lld\t%-20.20lld\t%-4.4x\n",
                       fiemap->fm_extents[i].fe_physical,
                       fiemap->fm_extents[i].fe_physical + fiemap->fm_extents[i].fe_length,
                       filename,
                       filelength,
                       fiemap->fm_extents[i].fe_logical,
                       fiemap->fm_extents[i].fe_length,
                       fiemap->fm_extents[i].fe_flags
                       );
            }
        }
        close(fd);

    }
    if (db)
    {
        sqlite3_finalize(stmt);
        sqlite3_exec(db,"COMMIT",NULL, NULL, &zErr);
        sqlite3_close(db);
        sqlite3_shutdown();
    }
    free(fiemap);
    fclose(listfile);
    return(0);
}

