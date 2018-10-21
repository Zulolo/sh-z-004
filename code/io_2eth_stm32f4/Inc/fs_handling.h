#ifndef _FS_HANDLING_H
#define _FS_HANDLING_H


void FS_remove_all_files(void);
void start_erase_all_files_task(void* argument);
void start_format_fs_task(void* argument);

#endif

