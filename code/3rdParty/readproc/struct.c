#include "struct.h"

/*
   struct.c

   The Library to struct.h, see this file for desciptions.
   (C) Daniel Knuettel 2014
   
   GNU General Public License v3

   This Library comes with ABSOLUTLY NO WARRANTY!
   See the GNU GPL v3 for further information.
 */

//#define DEBUG

void exitFatalOutOfMemory(void)
{
	printf("\n\n\tERROR: FATAL: OUT OF MEMORY!!!\n\nexiting\n");
	exit(-2);
}

void exitIOError(void)
{
	printf("\n\n\tERROR: IO ERROR:\texiting\n\n");
	exit(-3);
}
struct Job * newJob(
		char * name,
		unsigned int pid,
		unsigned int ppid,
		unsigned int uid,
		char status
	)
{
	struct Job * jb=malloc(sizeof(struct Job));
	if(!jb)
	{
		exitFatalOutOfMemory();
	}
	jb->name=malloc(sizeof(unsigned char)*sizeof(name));
	if(!jb->name)
	{
		exitFatalOutOfMemory();
	}
	strcpy(jb->name,name);
	jb->pid=pid;
	jb->ppid=ppid;
	jb->uid=uid;
	jb->status=status;
	return jb;
}

void set_name(struct Job * job, char * newName)
{
	strcpy(job->name,newName);
	return;
}
 struct Knot * newKnot(struct Job * job)
{
	struct Knot * knt= malloc(sizeof(struct Knot));
	if(!knt)
	{
 		exitFatalOutOfMemory();
	}
	knt->job=job;
	knt->got_next=EMPTY;
	return knt;
}

struct Root * newRoot(void)
{
	struct Root * rt=malloc(sizeof(struct Root));
	if(!rt)
	{
		exitFatalOutOfMemory();
	}
	rt->len=0;
	rt->got_first=EMPTY;
	return rt;
}

short int deleteKnot(struct Knot * knot)
{
	free(knot);
	if(!knot)
	{
		return 1;
	}
	return -1;
}
short int deleteKnotComplete(struct Knot * knot)
{
	knot->next=NULL;
	if(1!=deleteJobComplete(knot->job))
	{
		return -1;
	}
	free(knot);
	if(knot)
	{
		return -1;
	}
	return 1;
}

short int deleteRoot(struct Root * root)
{
	struct Knot * varknot;

	while(root->len--!=0)
	{
		varknot=root->first;
		root->first=root->first->next;
		if(1!=deleteKnot(varknot))
		{
			return -1;
		}
	}
	root->last=NULL;
	free(root);
	if(root)
	{
		return -1;

	}
	return 1;
}

short int deleteRootComplete(struct Root * root)
{

	struct Knot * varknot;

	while(root->len--!=0)
	{
		varknot=root->first;
		root->first=root->first->next;
		if(1!=deleteKnotComplete(varknot))
		{
			return -1;
		}
	}
	root->last=NULL;
	free(root);
	if(root)
	{
		return -1;

	}
	return 1;

}


short int deleteJobComplete(struct Job * job)
{
	free(job->name);
	if(job->name)
	{
		return -1;
	}
	free(job);
	if(job)
	{
		return -1;
	}
	return 1;
}

short int append(struct Root * root, struct Job * job)
{
	if(!root->got_first&FULL) 	//if the list is empty
	{
		root->first=newKnot(job);
		root->last=root->first;
		root->got_first=FULL;
		root->len++;
		return 0;
	}
	root->last->next=newKnot(job);
	root->last->got_next=FULL;
	root->last=root->last->next;
	root->len++;
	return 1;
}


struct Job * get_from_place( struct Root * root, unsigned long int place)
{
	if(place>root->len)
	{
		printf("\nERROR: place too high!\n");
		return;
	}
	unsigned long int count=0;
	struct Knot * varknot=root->first;
	while(count++!=place)
	{
		varknot=varknot->next;
	}
	return varknot->job;
}

struct Job * remove_from_place( struct Root * root, unsigned long int place)
{
	#ifdef DEBUG
	printf("DEBUG: running  remove_from_place(0x%x, %lu)\n",root,place);
	#endif
	if(place>root->len)
	{
		printf("\nERROR: place too high!\n");
		return;
	}
	if(place==0)
	{
		struct Knot * knt=root->first;
		root->first=root->first->next;
		if(!knt->got_next&FULL)
		{
			root->got_first=EMPTY;
		}
		return knt->job;
	}
	unsigned long int count=0;
	struct Knot * varknot, * lastknot;
	varknot=root->first;
	while(count++!=place)
	{
		lastknot=varknot;
		varknot=varknot->next;
	}
	lastknot->next=varknot->next;
	struct Job * ret=varknot->job;
	deleteKnot(varknot);
	root->len--;
	#ifdef DEBUG
	printf("DEBUG: ran remove_from_place() returning 0x%x\n",ret);
	#endif
	return ret;
}
