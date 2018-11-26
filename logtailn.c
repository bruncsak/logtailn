/* --------------------------------------------------------------------*/
/* logtailn.c -- multi file tail program that remembers last position. */
/*                                                                     */
/* Author: Attila Bruncsak                                             */
/*                                                                     */
/* Please send me any hacks/bug fixes you make to the code. All        */
/* comments are welcome!                                               */
/*                                                                     */
/* Based upon the work of Craig H. Rowland, which is then:             */
/*                                                                     */
/* Idea for program based upon the retail utility featured in the      */
/* Gauntlet(tm) firewall protection package published by Trusted       */
/* Information Systems Inc. <info@tis.com>                             */
/*                                                                     */
/* This program will read in a standard text files and create an       */
/* offset marker when it reads the end. The offset marker is read      */
/* the next time logtailn is run and the text file pointer is moved    */
/* to the offset location. This allows logtailn to read in the next    */
/* lines of data following the marker. This is good for marking log    */
/* files for automatic log file checkers to monitor system events.     */
/*                                                                     */
/* This program covered by the GNU License. This program is free to    */
/* use as long as the above copyright notices are left intact. This    */
/* program has no warranty of any kind.                                */
/*                                                                     */
/* VERSION 1.0: Initial release                                        */
/*                                                                     */
/* --------------------------------------------------------------------*/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/stat.h>

#define VERSION "1.0"

/* Tell them how to use this */
int usage(void)
  {
  fprintf(stderr, "\nlogtailn: version %s \n\n", VERSION);
  fprintf(stderr, "Written by Attila Bruncsak\n");
  fprintf(stderr, "Based upon Craig H. Rowland\n");
  fprintf(stderr, "Based upon original utility: retail (c)Trusted Information Systems\n");
  fprintf(stderr, "This program is covered by the GNU license.\n");
  fprintf(stderr, "\nUsage: logtailn [-o <offset_file>] <LOG_FILE> ... <LAST_LOG_FILE>\n");
  fprintf(stderr, "\nlogtailn will read in files and output to stdout.\n\n");
  fprintf(stderr, "After outputing the files, logtailn will create a file called\n");
  fprintf(stderr, "<LAST_LOG_FILE>.offset in the same directory that will contain the\n");
  fprintf(stderr, "decimal offset and inode of the file in ASCII format. \n\n");
  fprintf(stderr, "Next time logtailn is run the offset file is read and\n");
  fprintf(stderr, "output begins at the saved offset.\n\n");
  fprintf(stderr, "Rotated log files will be automatically accounted for by having\n");
  fprintf(stderr, "the offset reset to zero.\n\n");
  fprintf(stderr, "The optional <offset_file> parameter can be used to specify your\n");
  fprintf(stderr, "own name for the offset file. \n\n");
  return EX_USAGE;
  }

char * makeoffsetfilename(char * fname)
  {
  char * p;
  if ( (p = malloc(strlen(fname) + 8)) == NULL)
    return p;
  strcpy(p, fname);
  strcat(p, ".offset");
  return p;
  }

int readoffsetfile(char * offset_filename, unsigned long * inode, long * offset_position)
  {
  FILE *offset_output; /* name of the offset output file */
  int retval;

  /* see if we can open an existing offset file and read in the inode */
  /* and offset */
  if((offset_output = fopen(offset_filename, "rb")) != NULL)
    { /* read in the saved inode number and file position */
    if ((retval = fscanf(offset_output, "%lu %ld ", inode, offset_position)) == EOF
      || retval != 2
      || !feof(offset_output))
      {
      fprintf(stderr, "Invalid offset file format\n");
      fclose(offset_output); /* We're done, clean up */
      return EX_DATAERR;
      }
    fclose(offset_output); /* We're done, clean up */
    }
  else /* can't read the file? then assume no offset file exists */
    {
    *inode = 0L;
    *offset_position = 0L; /* if the file doesn't exist, assume */
                           /* offset of 0 because we've never */
                           /* tailed it before */
    }
  return EX_OK;
  }

int fileoutput(char * logname, long offset_position, long * new_offset_position)
  {
  char buffer[BUFSIZ];            /* I/O Buffer */
  unsigned int n;
  FILE *input;  /* Value user supplies for input file */

  /* Check if the file exists in specified directory */
  /* Open as a binary in case the user reads in non-text files */
  if ((input = fopen(logname, "rb")) == NULL)
    {
    fprintf(stderr, "File %s cannot be read.\n", logname);
    return EX_NOINPUT;
    }
  if (offset_position != 0L)
    fseek(input, offset_position, SEEK_SET); /* set the input file stream to */
                                             /* the offset position */
  /* Print the file */
  while ((n = fread(buffer, 1, BUFSIZ, input)) != 0)
    if (fwrite(buffer, 1, n, stdout) != n)
      {
      *new_offset_position = ftell(input); /* set new offset */
      fclose(input); /* clean up */
      return EX_IOERR;
      }
  *new_offset_position = ftell(input); /* set new offset */
  fclose(input); /* clean up */
  return EX_OK;
  }

int writeoffsetfile(char * offset_filename, unsigned long inode, long offset_position)
  {
  FILE *offset_output; /* name of the offset output file */
  if((offset_output = fopen(offset_filename, "w")) == NULL)
    {
    fprintf(stderr, "File %s cannot be created. Check your permissions.\n", offset_filename);
    return EX_CANTCREAT;
    }
  else
    {
    if ((chmod(offset_filename, 00600)) != 0) /* Don't let anyone read offset */
      {
      fprintf(stderr, "Cannot set permissions on file %s\n", offset_filename);
      return EX_NOPERM;
      }
    else
      fprintf(offset_output, "%ld %ld\n", inode, offset_position); /* write it */
    }
  fclose(offset_output);
  return EX_OK;
  }

int main(int argc, char *argv[])
  {
  char * offset_filename, * logfname;
  int status, startargc, i;
  unsigned long inode;
  long position, newposition;
  struct stat file_stat;

  if ( argc < 2 )
    return usage();
  else if ( strcmp(argv[1], "-o") == 0 )
    if ( argc < 4 )
      return usage();
    else
      {
      offset_filename = argv[2];
      startargc = 3;
      }
  else
    {
    offset_filename = makeoffsetfilename(argv[argc-1]);
    startargc = 1;
    }
  if ((status = readoffsetfile(offset_filename, &inode, &position)) != EX_OK)
    return status;
  for ( i = startargc; i < argc; i++ )
    {
    if((stat(argv[i], &file_stat)) != 0) /* load struct */
      {
      perror(argv[i]);
      return EX_DATAERR;
      }
    if ( inode == file_stat.st_ino )
       break;
    }
  if (i == argc)
    {
    /* previous log file not found, all must go to the output */
    i = startargc;
    position = 0L;
    }
  else if ( position > file_stat.st_size )
    {
    position = 0L; /* reset offset and report everything */
    fprintf(stderr, "***************\n");
    fprintf(stderr, "*** WARNING ***: Log file %s is smaller than last time checked!\n", argv[i]);
    fprintf(stderr, "***************         This could indicate tampering.\n");
    }
  for (; i < argc; i++)
    {
    logfname = argv[i];
    if ((status = fileoutput(logfname, position, &newposition)) != EX_OK)
      return status;
    position = 0L; /* for the next file if any start the output from the beginning */
    }
  /* after we are done we need to write the new offset */
  if((stat(logfname, &file_stat)) != 0) /* load struct */
    {
    perror(logfname);
    return EX_DATAERR;
    }
  return writeoffsetfile(offset_filename, file_stat.st_ino, newposition);
  }
