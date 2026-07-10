/* See LICENSE file for copyright and license details.
 *
 * FIFO-based IPC: a named pipe (fifopath, config.h) accepts newline
 * terminated "cmd [arg]" lines and dispatches them through the fifocmds[]
 * table in ipc.c. See DOCS.md / WIKI.md for the current command list.
 *
 * Query side: the "state" command writes a one-line status dump to a
 * second named pipe (fiforeplypath, config.h) instead of blindly guessing
 * dwm's state from outside. Read it with e.g. `cat /tmp/dwm.fifo.reply`
 * right after sending `state` to fifopath.
 */
#ifndef IPC_H
#define IPC_H

/* run()'s event loop polls this each tick: `if (fifofd >= 0) readfifo();` */
extern int fifofd;
/* only touched here and in dwm.c's cleanup() */
extern int fiforeplyfd;

void setupfifo(void); /* called once from setup() */
void readfifo(void);  /* called from run() whenever fifofd is valid */

#endif /* IPC_H */