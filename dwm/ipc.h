/* See LICENSE file for copyright and license details.
 *
 * FIFO-based IPC: a named pipe (fifopath, config.h) accepts newline
 * terminated "cmd [arg]" lines and dispatches them through the fifocmds[]
 * table in ipc.c. See DOCS.md / WIKI.md for the current command list.
 */
#ifndef IPC_H
#define IPC_H

/* run()'s event loop polls this each tick: `if (fifofd >= 0) readfifo();` */
extern int fifofd;

void setupfifo(void); /* called once from setup() */
void readfifo(void);  /* called from run() whenever fifofd is valid */

#endif /* IPC_H */
