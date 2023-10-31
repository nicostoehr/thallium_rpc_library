#ifndef CUSTOM_STRUCT
#define CUSTOM_STRUCT

#ifndef STAT_H
#define STAT_H 
#include <sys/stat.h>
#endif
//#include <custom_struct.cpp>


class custom_struct {
    

    
    public: 

        //dev_t       cst_dev;     /* NOT NEEDED: ID of device containing file */
        unsigned long     cst_ino;     /* inode number */ 
        unsigned int      cst_mode;    /* protection */ 
        unsigned long     cst_nlink;   /* number of hard links */
        uid_t       cst_uid;     /* user ID of owner */
        gid_t       cst_gid;     /* group ID of owner */
        //dev_t       cst_rdev;    /* NOT NEEDED: device ID (if special file) */
        off_t       cst_size;    /* total size, in bytes */
        unsigned int      cst_atime_tv_sec;   /* time of last access */
        unsigned long      cst_atime_tv_nsec;
        unsigned int cst_mtime_tv_sec; /* time of last modification */
        unsigned long cst_mtime_tv_nsec;
        unsigned int      cst_ctime_tv_sec;   /* time of last status change */
        unsigned long      cst_ctime_tv_nsec;
        blksize_t   cst_blksize; /* blocksize for filesystem I/O */
        blkcnt_t    cst_blocks;  /* number of blocks allocated */

        int return_value;


        custom_struct() = default;

        custom_struct(struct stat sb, int ret_val=0){
            //cst_dev = sb.st_dev;     /* ID of device containing file */
            cst_ino = (unsigned long) sb.st_ino;     /* inode number */
            cst_mode = (unsigned int) sb.st_mode;    /* protection */
            cst_nlink = (unsigned long) sb.st_nlink;   /* number of hard links */
            cst_uid = sb.st_uid;     /* user ID of owner */
            cst_gid = sb.st_gid;     /* group ID of owner */
            //cst_rdev = sb.st_rdev;    /* device ID (if special file) */
            cst_size = sb.st_size;    /* total size, in bytes */
            cst_atime_tv_sec = (unsigned int) sb.st_atim.tv_sec;   /* time of last access */
            cst_atime_tv_nsec = (unsigned long) sb.st_atim.tv_nsec;
            cst_mtime_tv_sec = (unsigned int) sb.st_mtim.tv_sec;   /* time of last modification */
            cst_mtime_tv_nsec = (unsigned long) sb.st_mtim.tv_nsec;
            cst_ctime_tv_sec = (unsigned int) sb.st_ctim.tv_sec; /* time of last status change */
            cst_ctime_tv_nsec = (unsigned long) sb.st_ctim.tv_nsec;
            cst_blksize = sb.st_blksize; /* blocksize for filesystem I/O */
            cst_blocks = sb.st_blocks;  /* number of blocks allocated */    

            return_value = ret_val; 
        }


        template<typename A> 
        void serialize(A& ar){
            //ar & cst_dev;     /* ID of device containing file */
            ar & cst_ino;     /* inode number */
            ar & cst_mode;    /* protection */
            ar & cst_nlink;   /* number of hard links */
            ar & cst_uid;     /* user ID of owner */
            ar & cst_gid;     /* group ID of owner */
            //ar & cst_rdev;    /* device ID (if special file) */
            ar & cst_size;    /* total size, in bytes */
            ar & cst_atime_tv_sec;   /* time of last access */
            ar & cst_atime_tv_nsec;
            ar & cst_mtime_tv_sec;   /* time of last modification */
            ar & cst_mtime_tv_nsec;
            ar & cst_ctime_tv_sec;   /* time of last status change */
            ar & cst_ctime_tv_nsec;
            ar & cst_blksize; /* blocksize for filesystem I/O */
            ar & cst_blocks;  /* number of blocks allocated */     

            ar & return_value;
    }

        struct stat get_struct(){
            struct stat sb;
            //sb.st_dev = cst_dev;    
            sb.st_ino = (ino_t) cst_ino;     
            //sb.st_mode = (mode_t) cst_mode;  
            sb.st_mode = static_cast<mode_t> (cst_mode);    
            sb.st_nlink = (nlink_t) cst_nlink;   
            sb.st_uid = cst_uid;     
            sb.st_gid = cst_gid;     
            //sb.st_rdev = cst_rdev;    
            sb.st_size = cst_size;

            struct timespec times;
            times.tv_sec = (time_t) cst_atime_tv_sec;  
            times.tv_nsec = (long) cst_atime_tv_nsec; 
            sb.st_atim = times;
             
            struct timespec times2;
            times2.tv_sec = (time_t) cst_mtime_tv_sec;  
            times2.tv_nsec = (long) cst_mtime_tv_nsec; 
            sb.st_mtim = times2;

            struct timespec times3;
            times3.tv_sec = (time_t) cst_ctime_tv_sec;  
            times3.tv_nsec = (long) cst_ctime_tv_nsec; 
            sb.st_ctim = times3;
  
            sb.st_blksize = cst_blksize; 
            sb.st_blocks = cst_blocks;  
            return sb; 
        }
        
};








#endif
