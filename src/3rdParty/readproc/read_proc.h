#include"struct.h"
#include<dirent.h>
#include<stdlib.h>
#include<stdio.h>

/*
   read_proc.h

   Reads the jobs from the /proc directory.
   Some of the methods like is_uint() should be elsewhere,
   but I wanted to keep it a bit smaller.

   (C) Daniel Knuettel 2014

   GNU General Public License v3

   This program comes with ABSOLUTLY NO WARRANTY!
   See the GNU GPL v3 for further information.
 */

#ifndef _READPROC_H
#define _READPROC_H

#define DIRNAME_BUF_LEN		20
#define BUF_LEN			30
/* the length of the buffer for the directory names 
   10 is in /proc  pretty good, because the biggest name is
   "version_signature".
 */

/*
   read all Jobs from the /proc. 
   returns a list of the Jobs,
   these are stored in the struct Root.
 */
struct Root * read_proc(void);

/*
   test if a string is an int.
   returns 1 if it is an int,
   else it returns 0.
   works only on unsigned int,
   modifiers long and short work, too.

 */
int  is_uint(char input[]);
/*
   read the needed parameters for
   newJob() of the file "/proc/<pid>/status",
   returns an new Job.
 */
struct Job * get_job(char * path);

/*
   Searches for a Job with the name 'name'.
   Returns 1 if it is present,0 if it is not present
   and -1 if an error occurred.
 */
short int is_present(char * name);

#endif
