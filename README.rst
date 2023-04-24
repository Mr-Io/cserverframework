Light C Server Framework 
========================
.. ini-badges

.. todo: add shields (status bars (travis), code style, tech/framework used, test coverage…)

.. image:: https://img.shields.io/badge/language-C17-blue
  :target: https://en.cppreference.com/w/cpp/17

.. image:: https://img.shields.io/badge/OS-linux-orange
  :target: https://gcc.gnu.org/

.. image:: https://img.shields.io/github/license/Mr-Io/c-language-solutions
  :target: https://choosealicense.com/licenses/mit/

.. end-badges


.. ini-intro

**A very light C framework for making 
client-server programs based on tcp sockets.**

This repository came with a module 
for input/output on socket file descriptors. 
It has a similar interface as the
standard input/output library ``stdio``
(it works with a struct similar to FILE streams).

It is intended to replace ``open``/``read``/``write`` system calls,
and remove the need for ``getaddrinfo``/``getnameinfo`` and similar.

Features:

* Thread-safe.
* Buffered input [#]_.
* Full duplex, input and output may be interleaved without problem.
* No short-counts on read/write, blocking instead [#]_.
* No error on signal interrupts.
* Ipv4/Ipv6 agnostic 
  (except when choosing the type of listening socket).
 
.. [#] To Reduce expensive ``read`` system calls.
.. [#] This is the desired behaviour for a 
       multiprocessing/multithreading environment. 
       However, for IO multiplexing it is neccesary 
       to work directly with file descriptors.

Quick Example
-------------
Sending a message to a server and reading the response line by line 
can be done as follow:

.. code-block:: c

  TCPSOCKET *sstream;
  sstream = tcpsocket_connect("hostname/ip-address", "service/port");
  
  char buf[1024] = "hello server!";
  tcpwrite(sstream, "hello server!", sizeof(buf));

  int len;
  while((len = tcpread(sstream, '\n', buf, 1024)) > 0)
    fwrite(buf, 1, len, stdout);
  
  tcpsocket_close(sstream);

Some notes:

  * No error checking is done in the example. It should be done
    in production code.
  * Every ``tcpwrite`` call maps to a single write call, try to
    write as much as possible on every call.
  * The above statement is not true for tcpread;
    input is buffered, read is called only when needed and trying
    to read as much data as possible on each call. The total data
    limit depend on the internal buffer size of TCPSOCKET struct,
    (8192 bytes) 
  * TCPSOCKET must be closed in long running programs to avoid
    memory leakage.

Usage
-----
The repository has a very simple structure, all source files are 
on the main directory. No need for a build system or even a makefile.

To see the code for a bare-bones implementation example of a static web
server using the framework, checkout the following branches:

* webserver_example_iterative_: The simplest one, no concurrency.
* webserver_example_multiprocessing_: Using processes for concurrency.
* webserver_example_multithreading_: Using threads for concurrency.
* webserver_example_prethreading_: Using a producer-consumer approach 
  with threads for concurrency.

.. _webserver_example_iterative: https://github.com/Mr-Io/cserverframework/tree/webserver_example_iterative
.. _webserver_example_multiprocessing: https://github.com/Mr-Io/cserverframework/tree/webserver_example_multiprocessing
.. _webserver_example_multithreading: https://github.com/Mr-Io/cserverframework/tree/webserver_example_multithreading
.. _webserver_example_prethreading: https://github.com/Mr-Io/cserverframework/tree/webserver_example_prethreading

To test the example just compile the source with gcc:

.. code-block:: console

  git checkout webserver_example_#####
  gcc -o prog *.c
  mkdir static
  # populate static folder with html files (it uses index.html as origin)
  prog <port> # start listening on specified port
