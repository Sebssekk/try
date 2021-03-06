#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <signal.h>
#include <semaphore.h>
#include <string.h>
#include <mqueue.h>
#include <time.h>
#include "itostr.h"

sem_t * sem_id; // semaphore definition

/* message structure for messages in the shared segment */
typedef struct shared_data {
    int var1;
} shared_data;

void clean_up(int signum,char* SHMOBJ_PATH,char* SEM_PATH)
{
  printf("clean_up is working...\n" );
  /**
   * Semaphore unlink: Remove a named semaphore  from the system.
   */
    if ( shm_unlink(SHMOBJ_PATH) < 0 )
    {
      perror("shm_unlink");
    }

  /**
   * Semaphore Close: Close a named semaphore
   */
    if ( sem_close(sem_id) < 0 )
    {
      perror("sem_close");
    }

  /**
   * Semaphore unlink: Remove a named semaphore  from the system.
   */
    if ( sem_unlink(SEM_PATH) < 0 )
    {
      perror("sem_unlink");
    }
   // Terminate program
    exit(signum);
}

int main(int argc,char* argv[])
{
  if (argc ==1)
  {
    printf("No args given....");
    return -1;
  }

  char SHMOBJ_PATH[15];
  strcpy(SHMOBJ_PATH,"/myregion");
  strcat(SHMOBJ_PATH,argv[1]);
  int shmfd; //frame descriptor ()
  int shared_seg_size = (1 * sizeof(shared_data));   /* want shared segment capable of storing 1 message */
  shared_data* shared_msg; /* the shared segment, and head of the messages list */

/* creating the shared memory object    --  shm_open()  */

  shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
  if (shmfd < 0)
  {
    perror("In shm_open()");
    exit(1);
  }
  fprintf(stderr, "Created shared memory object %s\n", SHMOBJ_PATH);

/* adjusting mapped file size (make room for the whole segment to map)      --  ftruncate() */

  ftruncate(shmfd, shared_seg_size);
	/**
	 * Semaphore open
	 */
  char SEM_PATH[15];
  strcat(SEM_PATH,"/mysem");
  strcat(SEM_PATH,argv[1]); //a sem for each region
	sem_id=sem_open(SEM_PATH, O_CREAT, S_IRUSR | S_IWUSR, 1);
  fprintf(stderr, "Created shared semaphore object %s\n", SEM_PATH);
/* requesting the shared segment    --  mmap() */

  shared_msg = (shared_data *)mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
  //with address as NULL it's OS who choose how to allocate shared memory
  if (shared_msg == NULL)
  {
    perror("In mmap()");
    exit(1);
  }
  fprintf(stderr, "Shared memory segment allocated correctly (%d bytes).\n", shared_seg_size);


  /*My code*/
  if (argc >2)
  {
    if (*argv[2]=='d')
    {
      printf("Clean up..\n" );
      clean_up(1,SHMOBJ_PATH,SEM_PATH);
      return -1;
    }
    else
    {
      sem_wait(sem_id);

      shared_msg->var1 = atoi(argv[2]);
      printf("I'm Message_Handler and i should have allocated : %d\n",atoi(argv[2]) );

      sem_post(sem_id);
    }
  }
  else
  {
    srand(time(NULL));
    printf("I'm Message_Handler and I was called with 1 arguent...\n" );
    sem_wait(sem_id);

    mqd_t mqd;
    struct mq_attr attr = {.mq_maxmsg = 10, .mq_msgsize = 50};
    char buffer[attr.mq_msgsize];
    char msg[10];
    int prio = rand () % 3;
    itostr(shared_msg->var1,msg);
    strncpy (buffer, msg, sizeof(buffer));
    char QUE_PATH[15];
    strcpy(QUE_PATH,"/queue");
    strcat(QUE_PATH,argv[1]);
    if ((mqd = mq_open(QUE_PATH, O_CREAT | O_RDWR, 0666, &attr)) == -1)
    {
        perror("mq_open");
        exit(1);
    }
    if (mq_send(mqd, buffer, sizeof(buffer), prio) == -1)
    {
        perror("mq_send");
        exit(1);
    }

    mq_close(mqd);

    sem_post(sem_id);
  }
  return 0;
 }
