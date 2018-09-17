#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <spawn.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include <sys/wait.h>
#include <sys/mman.h>

static const uint32_t default_iteractions  = 10000;
static const uint32_t default_rss_size     = 16 * 1024 * 1024;	// 16MB
static const char     dummy_program_name[] = "/bin/true";

static uint32_t
parse_uint32 (const char *arg)
{
  unsigned long int ret = strtoul (arg, NULL, 10);
  if ((errno == ERANGE) || (ret > UINT32_MAX))
    {
      fprintf (stderr, "error: invalide input argument: %s\n", arg);
      exit (EXIT_FAILURE);
    }
  return ret;
}

enum {
  BENCH_NONE        = 0x0,
  BENCH_POSIX_SPAWN = 0x1,
  BENCH_FORK_EXEC   = 0x2,
  BENCH_VFORK_EXEC  = 0x4,
  BENCH_ALL = BENCH_POSIX_SPAWN | BENCH_FORK_EXEC | BENCH_VFORK_EXEC
};

static int
parse_bench (const char *arg)
{
  if (strcmp (arg, "posix_spawn") == 0)
    return BENCH_POSIX_SPAWN;
  if (strcmp (arg, "fork_exec") == 0)
    return BENCH_FORK_EXEC;
  if (strcmp (arg, "vfork_exec") == 0)
    return BENCH_VFORK_EXEC;
  return BENCH_NONE;
}

typedef pid_t (*spawn_function_t) (const char *);

static pid_t
spawn_posix_function (const char *prog)
{
  pid_t pid;
  int status = posix_spawn (&pid, dummy_program_name, NULL, NULL,
			    (char *const[]){ (char*) dummy_program_name, NULL },
			    NULL);
  if (status != 0)
    {
      perror ("error: posix_spawn: ");
      exit (EXIT_FAILURE);
    }
  return pid;
}

static pid_t
fork_exec_function (const char *prog)
{
  pid_t pid = fork ();
  if (pid == 0)
    {
      if (execlp (dummy_program_name, "", NULL) == -1)
        perror ("error: execv: ");
      exit (EXIT_FAILURE);
    }

  return pid;
}

static pid_t
vfork_exec_function (const char *prog)
{
  pid_t pid = vfork ();
  if (pid == 0)
    {
      if (execlp (dummy_program_name, "", NULL) == -1)
        perror ("error: execv: ");
      exit (EXIT_FAILURE);
    }

  return pid;
}

static void
run (const char *title, uint32_t iteractions, uint32_t rss_size,
     spawn_function_t function)
{
  /* Allocated buffer to emulate a RSS usage).  */
  void *buffer = mmap (NULL, rss_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (buffer == MAP_FAILED)
    {
      perror ("error: mmap_failed: ");
      exit (EXIT_FAILURE);
    }
  memset (buffer, 0x55, rss_size);

  double max = DBL_MIN;
  double min = DBL_MAX;
  double total = 0.0;

  for (uint32_t i=0; i<iteractions; i++)
    {
      struct timespec ts_after;
      struct timespec ts_before;

      clock_gettime (CLOCK_REALTIME, &ts_after);

      pid_t pid = function (dummy_program_name);
      if (waitpid(pid, NULL, 0) == -1)
        {
          perror ("warning: waitpid failure: ");
          exit (EXIT_FAILURE);
        }
      clock_gettime (CLOCK_REALTIME, &ts_before);

      double accum = (((ts_before.tv_sec - ts_after.tv_sec) * 1e9)
                     + ( ts_before.tv_nsec - ts_after.tv_nsec ));

      if (accum > max)
        max = accum;
      if (accum < min)
        min = accum;

      total += accum;
    }

  printf ("%s\n", title);
  printf ("  total: %lf\n", total/1e9);
  printf ("  max:   %lf\n", max/1e9);
  printf ("  min:   %lf\n", min/1e9);
  printf ("  avg:   %lf\n", (total / iteractions)/1e9);
 
  munmap (buffer, rss_size);
}

static void
print_usage (const char *prog)
{
  printf ("usage: %s [OPTIONS]\n", prog);
  printf ("  -b          benchmarks to run: posix_spawn, fork_exec, or vfork_exec (default all)\n");
  printf ("  -i          number of iteractions (default %i)\n",
     default_iteractions);
  printf ("  -r          memory to allocate (default %i)\n",
     default_rss_size);
  exit (EXIT_SUCCESS);
}

int
main (int argc, char *argv[])
{
  int opt;
  uint32_t iteractions = default_iteractions;
  uint32_t rss_size = default_rss_size;
  int benchmark = BENCH_ALL;

  while ((opt = getopt (argc, argv, "i:r:hb:")) != -1)
    {
      switch (opt)
	{
	case 'i':
	  iteractions = parse_uint32 (optarg);
	  break;
	case 'r':
	  rss_size = parse_uint32 (optarg);
	  break;
	case 'b':
	  benchmark = parse_bench (optarg);
	  break;
        case 'h':
          print_usage (argv[0]);
	  break;
	}
    }

  if (benchmark & BENCH_POSIX_SPAWN)
    run ("posix_spawn", iteractions, rss_size, spawn_posix_function);
  if (benchmark & BENCH_FORK_EXEC)
    run ("fork", iteractions, rss_size, fork_exec_function);
  if (benchmark & BENCH_VFORK_EXEC)
    run ("vfork", iteractions, rss_size, vfork_exec_function);

  return 0;
}
