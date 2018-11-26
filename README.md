logtailn.c -- multi log file tail program that remembers last position.

Usage: logtailn [-o <offset_file>] <LOG_FILE> ... <LAST_LOG_FILE>

The program will read in a standard text log files and outputs to stdout.

After reading the end and outputing the files, logtailn will create an offset marker file called <LAST_LOG_FILE>.offset in the same directory that will contain the decimal offset and inode of the file in ASCII format.

The offset marker is read the next time logtailn is run and the text file pointer is moved to the offset location. This allows logtailn to read in the next lines of data following the saved marker. This is good for marking log files for automatic log file checkers to monitor system events.

Rotated log files will be automatically accounted for by having the offset reset to zero for the new log file.

The order of the log files is important: the oldest should be the first and the newest listed as the last argument.

The optional <offset_file> parameter can be used to specify your own name for the offset file.

A typical use:
````bash
logtailn -o my.offset $(ls -tr /var/log/maillog* | egrep '/maillog([.-][0-9]+)?$')
````

Written by Attila Bruncsak
Based upon the work of Craig H. Rowland

This program is covered by the GNU license.
