# Plans:
# 
#  The log files should be tempfiles for each process, then at the
#  end of the process, if the process is told to save the logs, then
#  the file should be moved to its final place. <- then processes
#  that fail will never produce logs..
# 
#  The log files should be removed entirely, that operation is left
#  to the user because they know they are using a parallel map 
#  process that could result in out-of-order prints.
# 
#  The number of arguments passed to processes should be reduced, if
#  possible the global QUEUEs should be used.
# 

import os, sys
# Get functions required for establishing multiprocessing
from multiprocessing import cpu_count, get_context, current_process
# Imports that are used by the processes.
from queue import Empty
# Import pickle for serializing objects.
from pickle import dumps, loads

# Make sure new processes started are minimal (not complete copies)
MAX_PROCS = cpu_count()
LOG_DIRECTORY = os.path.abspath(os.path.curdir)
# Global storage for processes
MAP_PROCESSES = []
# Create multiprocessing context (not to muddle with others)
MAP_CTX = get_context() # "fork" on Linux, "spawn" on Windows.
# Create a job_queue for passing jobs to processes
JOB_QUEUE = MAP_CTX.Queue()
JOB_GET_TIMEOUT = 0.1
# Create return queue for holding results
RETURN_QUEUE = MAP_CTX.Queue()

# Get a reference to the builtin "map" function, so it is not lost
# when it is overwrittend by "def map" later in this file.
builtin_map = map


# A parallel implementation of the builtin `map` function in
# Python. Provided a function and an iterable that serializable with
# "pickle", use all the cores on the current system to map "func" to
# all elements of "iterable".
# 
# INPUTS
#   func     -- Any function that takes elements of "iterable" as the first argument.
#   iterable -- An iterable object The elements of this will be passed into "func".
# 
# OPTIONALS:
#   max_waiting_jobs    -- Integer, maximum number of jobs that can be
#                          in the job queue at any given time.
#   max_waiting_returns -- Integer, maximum number of return values in
#                          the return queue at any given time.
#   order     -- Boolean, True if returns should be made in same order
#                as the originally generated sequence.
#                WARNING: Could potentially hold up to len(iterable)
#                return values from "func" in memory.
#   save_logs -- Boolean, True if log files from individual calls to
#                "func" should *not* be deleted.
#   redirect  -- Boolean, True if log files should be created per
#                process, otherwise all print goes to standard out.
#   threads   -- Number of threads to use in this parallel map process.
#   nchunks   -- Number of chunks to break the iterable into before
#                sending to children proceses. By default, no chunking
#                is done to save memory, if a whole number is provided
#                then the provided iterable will be converted into
#                nchunks separate Python lists (will take more memory).
#   args      -- Tuple or List of additional arguments to pass to "func"
#                after elements of iterable (via "*args").
#   kwargs    -- Dictionary of keyword arguments to be given to "func"
#                via "**kwargs".
# 
# RETURNS:
#   An output generator, just like the builtin "map" function.
# 
# WARNINGS:
#   If iteration is not completed, then waiting idle processes will
#   still be alive. Call "parallel.killall()" to terminate lingering
#   map processes when iteration is prematurely terminated.
# 
#   The mapped function will not typically have access to global variables.
def map(func, iterable, max_waiting_jobs=1, max_waiting_returns=1,
        order=True, save_logs=False, redirect=True,
        threads=MAX_PROCS, nchunks=4*MAX_PROCS, args=[], kwargs={}):
    # If chunking should be done, then `split` the iterable.
    chunking = (nchunks is not None)
    if chunking:
        iterable = split(enumerate(iterable), count=nchunks)
        total_size = sum([len(i) for i in iterable])
    else: iterable = enumerate(iterable)
    # Create the arguments going to all subprocesses.
    producer_args = (iterable, JOB_QUEUE)
    consumer_args = (dumps(func), JOB_QUEUE, RETURN_QUEUE, redirect, 
                     chunking, args, kwargs)
    # Start a "producer" process that will add jobs to the queue,
    # create the set of "consumer" processes that will get jobs from the queue.
    producer_process = MAP_CTX.Process(target=producer, args=producer_args, daemon=True)
    consumer_processes = [MAP_CTX.Process(target=consumer, args=consumer_args, daemon=True)
                          for i in range(threads)]
    # Track (in a global) all of the active processes.
    MAP_PROCESSES.append(producer_process)
    for p in consumer_processes: MAP_PROCESSES.append(p)
    # Start the producer and consumer processes
    producer_process.start()
    for p in consumer_processes: p.start()
    # Construct a generator function for reading the returns
    def map_results():
        # Set up variables for returning items in order if desired.
        if order:
            idx = 0
            ready_values = {}
        # Keep track of the number of finished processes.
        stopped_consumers = 0
        # Continue picking up results until generator is depleted
        while (stopped_consumers < threads):
            i, value = RETURN_QUEUE.get()
            if (value == StopIteration):
                stopped_consumers += 1
            elif isinstance(value, Exception):
                # Kill all processes for this "map"
                for p in consumer_processes: 
                    p.terminate()
                producer_process.terminate()
                # Raise exception without deleting log files.
                raise(value)
            elif (not order): yield value                
            else: ready_values[i] = value
            # Yield all waiting values in order (if there are any).
            if order:
                while (idx in ready_values):
                    yield ready_values.pop(idx)
                    idx += 1
        # Join all processes (to close them gracefully).
        for p in consumer_processes:
            p.join()
            MAP_PROCESSES.remove(p)
        producer_process.join()
        MAP_PROCESSES.remove(producer_process)
        # Delete log files if the user doess not want them.
        if not save_logs: clear_logs()
    # Return all results in a generator object.
    return map_results()


# Iterator that pulls jobs from a queue and stops iterating once the
# queue is empty (and remains empty for "JOB_GET_TIMEOUT" seconds). On
# construction, requires a "job_queue" that jobs will come from.
class JobIterator:
    def __init__(self, job_queue): self.jq = job_queue
    def __iter__(self): return self
    def __next__(self):
        try: value = self.jq.get(timeout=JOB_GET_TIMEOUT)
        except Empty: raise(StopIteration)
        return value


# Given an iterable object and a queue, place jobs into the queue.
def producer(jobs_iter, job_queue):
    for job in jobs_iter:
        job_queue.put(job)


# Given an iterable object, a queue to place return values, and a
# function, apply "func" to elements of "iterable" and put return
# values into "return_queue".
# 
# INPUTS:
#   func         -- A function that takes elements of "iterable" as
#                   input. It is expected that the function has been
#                   packaged with with "dill.dumps" to handle lambda's.
#   job_queue    -- A Queue object with a 'get' method, holds jobs.
#   return_queue -- A Queue object with a 'put' method.
#   redirect     -- Boolean, True if log files should be created per
#                   process, otherwise all print goes to standard out.
#   chunked      -- True if iterable contains a list of argument lists,
#                   instead of just a single list of arguments.
#   args         -- Extra arguments to pass to "func" with "*args".
#   kwargs       -- Extra keyword arguments to pass to "func" with "**kwargs".
def consumer(func, job_queue, return_queue, redirect, chunked, args, kwargs):
    # Retrieve the function (because it's been serialized for transfer)
    func = loads(func)
    log_file_name = f"Process-{int(current_process().name.split('-')[-1].split(':')[0])-1}"
    if redirect:
        # Set the output file for this process so that all print statments
        # by default go there instead of to the terminal
        # log_file_name = current_process().name
        short_name = log_file_name
        log_file_name = os.path.join(LOG_DIRECTORY, log_file_name) + ".log"
        sys.stdout = open(log_file_name, "w")
    # If this is not chunked, then the "iterable" contains arguments.
    if not chunked: iterable = [JobIterator(job_queue)]
    else:           iterable = JobIterator(job_queue)
    # Assume that the master iterable contains multiple lists of arguments.
    for arg_list in iterable:
        # Iterate over values and apply function.
        for (i,arg) in arg_list:
            try:
                # Try executing the function on the value.
                return_queue.put((i,func(arg, *args, **kwargs)))
            except Exception as exception:
                # If there is an exception, put it into the queue for the parent.
                return_queue.put((i,exception))
                # Close the file object for output redirection
                if redirect: sys.stdout.close()
                return
    # Once the iterable has been exhaused, put in a "StopIteration".
    return_queue.put((-1, StopIteration))
    # Close the file object for output redirection
    if redirect: sys.stdout.close()


# Given an iterable, split it into as many chunks as there are
# processes. This is not particularly efficient, as it uses python
# lists, but it is convenient!
def split(iterable, count=MAX_PROCS):
    # Fill the chunks by walking through the iterable.
    chunks = [list() for i in range(count)]
    for i,v in enumerate(iterable):
        chunks[i%count].append( v )
    # Return the list of lists that are chunks.
    return chunks


# Clear out the log files from each process.
def clear_logs():
    import os
    for f_name in os.listdir(LOG_DIRECTORY):
        if (f_name[:8] == "Process-") and (f_name[-4:] == ".log"):
            f_name = os.path.join(LOG_DIRECTORY, f_name)
            os.remove(f_name)


# Kill all active "map" processes.
def killall():
    # Terimate all processes and empty global record.
    while len(MAP_PROCESSES) > 0:
        proc = MAP_PROCESSES.pop(-1)
        proc.terminate()

