#include<stdlib.h>
#include<string.h>
#include<stdio.h>


/*
   struct .h

   Contains all structs and methods for working with the
   /proc directory.
   You will use only the structs Job and Root really,
   the struct Knot is only there to combine these.
   I am using a single joined chain to store the Jobs.

   every Job has 
   	char * name   		wich describes the name of the program
	unsigned int pid 	wich is his pid
	unsigned int ppid	wich is his ppid
	unsigned int uid	the uid of the user wich started the job
	//this is currently the first field in the status file
	char status		the status of the job
   this is nearly everything tha we usually may use.

   (C) Daniel Knuettel 2014
   
   GNU General Public License v3

   This program comes with ABSOLUTLY NO WARRANTY!
   See the GNU GPL v3 for further information.
 */

#ifdef _parse_proc_struct_big
#error "parse_proc_struct is incompatible to parse_proc_struct_big! \nexiting"
#else
#ifndef _parse_proc_struct
#define _parse_proc_struct


#define FULL		0x01	// if Root or Knot is filled with a Knot
#define EMPTY		0x00
/*
   The struct Job has all the relevant 
   values of a job.
 */
struct Job
{
	char * name;		// the name of the job, as in comm
	unsigned int pid;	// the process' id
	unsigned int ppid;	// the parent's pid
	unsigned int uid;	// the user's id
	char status;		// the status like 'S' for sleep


};

/*
   The struct Knot combines the Jobs
 */

struct Knot
{
	unsigned char got_next;
	struct Knot * next;
	struct Job * job;
};

/*
   This struct is the Root of the complete list.
   It is there to hold the knots.
 */

struct Root
{
	unsigned long int len;
	struct Knot * first;
	unsigned char got_first;
	struct Knot * last;
};



// this is bad.
void exitFatalOutOfMemory(void);
// this is bad,too
void exitIOError(void);
// all the constructors
struct Knot * newKnot(struct Job * job);

struct Job * newJob(
		char * name, 
		unsigned int pid, 
		unsigned int ppid, 
		unsigned int uid,
		char status
	);

struct Root * newRoot(void);

// destructors

short int deleteKnot(struct Knot * knot);
short int deleteKnotComplete(struct Knot * knot);
short int deleteRoot(struct Root * root);
short int deleteRootComplete(struct Root * root);
short int deleteJobComplete(struct Job * job);

/*
   some methods you should use to work with the structs
 */

void set_name( struct Job * job, char * newName);
/*
   append an job to the list,
   returns 0, if it is the first knot, else 1
 */
short int append(struct Root * root, struct Job * job);

/*
   get the job from a specific place
 */

struct Job * get_from_place(struct Root * root, unsigned long int place);

/*
   remove the job from the place 
 */

struct Job * remove_from_place(struct Root * root, unsigned long int place);
#endif
#endif
