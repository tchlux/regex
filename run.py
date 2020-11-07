'''
                           run.py

Wrapper for running the regex module main program. This is largely
present to compartmentalize profiling for timing and memory usage.

'''

# Make sure the path includes this directory (because that will allow 
# Python to successfully import the correct "regex" module).
import os,sys
sys.path.insert(0,os.path.abspath(os.curdir))

# Run the "main" program for the regex module.
import regex
regex.main()
