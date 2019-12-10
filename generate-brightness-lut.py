#!/usr/bin/env python3

"""
This is a utility script for the Arduino Christmas lights sketch.

This is used to generate the integer lookup table for brightness values.
Simple example to generate a lookup table containing 100 values on Mac / Linux:

    ./generate-brightness-lut.py 100

On Windows, you may have to do this instead:

    python generate-brightness-lut.py 100

In the example, the second half of the lookup table will contain all zeroes.
That is because the output values are clipped to the range 0 to 255 (inclusive) by default.

To see complete usage information, execute the script with command line option --help.
For example, on Mac / Linux:

    ./generate-brightness-lut.py --help

On Windows, you may need to execute it like this:

    python generate-brightness-lut.py --help

Author: Peter Bloomfield
Website: https://peter.bloomfield.online
Copyright (C) 2019 Peter Bloomfield
Licensed under GNU GPL v3.
"""

import argparse
import math
import re
import sys

# Regular expression for matching a C++ type identifier possibly including scope resolution operators.
# For example, it will match "int", "::int8_t", and "std::uint16_t", but not "blah::::foo".
regex_cpp_identifier_with_scope = re.compile('^((?:::)?[a-zA-Z_][a-zA-Z0-9_]*)+$')

# Regular expression for matching a C++ variable name.
# For example, it will match "blah" and "foo123", but not "hello world!".
regex_cpp_identifier = re.compile('^[a-zA-Z_][a-zA-Z0-9_]*$')


# Define the commad line help info and arguments.
usage_description = """\
Generate a brightness lookup table for the Arduino Christmas lights sketch.

This will produce source code for a C/C++ style array containing the brightness values. \
Add the array to the sketch source code. \
"""
usage_epilog = """\
The lookup table is based on a sine wave. The basic formula is:

  y = (a * sin(f * (x - p))) + s

Where a is amplitude, f is frequency, p is phase, and s is vertical shift. \
The frequency indicates how many complete cycles of sine wave will fit into the lookup table. \
The decimal fractions are rounded to the nearest integer in the output.
"""

parser = argparse.ArgumentParser(description=usage_description, epilog=usage_epilog)

parser.add_argument('length', type=int, help='Number of entries in the lookup table.')
parser.add_argument('-d', '--data-type', type=str, default='byte', help='Data type to specify in the generated code.')
parser.add_argument('-n', '--name', type=str, default='BRIGHTNESS_LUT', help='Name of the array variable in the generated code.')
parser.add_argument('-a', '--amplitude', type=float, default=255.0, help='Amplitude of the sine wave to generate.')
parser.add_argument('-f', '--frequency', type=float, default=1.0, help='Number of complete sine waves to fit into the lookup table.')
parser.add_argument('-p', '--phase', type=float, default=0.0, help='Horizontal offset of the sine wave in radians. For example, pi will offset it by half')
parser.add_argument('-s', '--shift', type=float, default=0.0, help='Vertical offset of the sine wave.')
parser.add_argument('-l', '--lower-cutoff', type=int, default=0, help='Minimum valued in the output lookup table. Lower values will be clamped to this limit.')
parser.add_argument('-u', '--upper-cutoff', type=int, default=255, help='Maximum valued allowed in the output lookup table. Higher values will be clamped to this limit.')
parser.add_argument('-w', '--wrap', type=int, default=20, help='Number of values per line in the generated code. Set this to 0 to disable wrapping. Wrapping is ignored if --values-only is specified.')
parser.add_argument('-v', '--values-only', action='store_const', const=True, help='If this flag is specified, the output will only contain comma-separated values. It will skip other code generation.')

args = parser.parse_args()

# Sanity check the command line arguments.
if not args.values_only:
    if not regex_cpp_identifier_with_scope.match(args.data_type):
        raise ValueError('--data-type musdt specify a valid C++ type identifier.')

    if not regex_cpp_identifier.match(args.name):
        raise ValueError('--name must specify a valid C++ variable name.')

if args.lower_cutoff > args.upper_cutoff:
    raise ValueError('--lower-cutoff must not be greater than --upper-cutoff.')

# Generate the lookup table values.
lut = []
for index in range(args.length):
    # Spread a single cycle (2 pi radians) evenly across the lookup table.
    x = (float(index) / float(args.length)) * 2.0 * math.pi
    # Calculate the actual value for this position.
    y = (args.amplitude * math.sin(args.frequency * (x - args.phase))) + args.shift
    # Round the value and constrain it to our cutoffs
    constrained_y = max(args.lower_cutoff, min(args.upper_cutoff, int(round(y))))
    # Add the value to the LUT.
    lut.append(constrained_y)


if args.values_only:
    # ----- VALUES-ONLY OUTPUT ----- #
    for index in range(args.length):
        if index != 0:
            print(', ', end='')
        print(lut[index], end='')
    print('')
    
else:

    # ----- C++ CODE OUTPUT ----- #

    # Declare a constant containing the length of the lookup table.
    print('const int {0}_LENGTH{{ {1} }};'.format(args.name, args.length))
    # Open the declaration of the lookup table.
    print('const {0} {1}{{ '.format(args.data_type, args.name), end='')

    # Go through each item in the lookup table
    for index in range(args.length):
        
        # Add a separator after the previous value.
        if index > 0:
            print(', ', end='')

        # Wrap around if necessary and indent each line.
        # Note that this also puts a line break before the first line
        if args.wrap > 0 and (index % args.wrap) == 0:
            print('')
            print('  ', end='')

        print(lut[index], end='')

    # If wrapping was enabled then add a linebreak after the last value.
    if args.wrap > 0:
        print('')

    # Close the declaration of the lookup table.
    print('};')
