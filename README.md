# Semaphore Shared Memory example

This is an example of using semaphores to synchronize communication over
shared memory between two separate processes. The two processes, `server` and
`client`, use two semaphores and two shared memory segments to communicate.

The two semaphores are used to signal when each process is done accessing
shared memory.  The two shared memory segments are a "header" segment and a
"data" segment.  The header segment is used to describe the size and metadata
of the contents of the data segment.

## Communication overview

`server` will ask for a file from the client. The name of the file is chosen
at random, and the length (bytes) of this filename is placed in the header
segment. This is done so `client` knows how many bytes to read from the data
segment. The filename is then dropped into the data segment. `server`
increments its semaphore, letting `client` know that it's done. `server` then
waits to decrement the client semaphore; it is blocked until `client` is
finished.

`client` then reads the length of the filename from the header segment, and
then reads that many bytes from the data segment. With the filename known,
`client` loads the file. The size of the file is then placed into the header
segment, overwriting the previous value. In this case, since the file is an
image, the dimensions and number of channels are also written to the header
segment. `client` then writes the image file's bytes to the data segment and
increments its semaphore, letting `server` know the file is ready to be read.

Finally, `server` is unblocked and can read the header segment to determine
how to read the file in the data segment.

## Required knowledge

`server` and `client` need to know a few things to cooperate.

* The two semaphore names must be known by both processes
  * POSIX named semaphores are used because they exist as a file outside the
    processes memory spaces.  Unnamed semaphores cannot be used because they
    exist only within the current process' memory. System V semaphores are not
    used because they are ancient
* The two shared memory segment names must be known by both processes

## API

* The `server` command is a string whose length is placed in the header
  segment as a single `uint64_t`.
* The `client` response is an image file. The total number of bytes, the
  dimensions of the image, and the number of channels in the image are placed
  in the header segment as an array of 4 `uint64_t`
* Shared memory segments can be resized up, but not down. This shaves off a
  bit of time when varying size files are sent because the OS is not
  re-allocating shared memory with each command and response
