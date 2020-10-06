import sys
from .regex import match

# Function for printing the help message.
def print_help_message():
    print("""
Help message will go here.
    """)

# If no arguments were provided, give the usage.
if len(sys.argv) <= 4:
    print_help_message()
    exit()

print_help_message()
