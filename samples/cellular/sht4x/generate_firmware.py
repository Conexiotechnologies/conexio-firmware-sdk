##################################################
## {Description}
# This script generates the required binaries
# for the Conexio Stratus IoT kit
##################################################
# Copyright (c) 2021 Conexio Technologies, Inc
##################################################
# Using system() method to execute shell commands

# To execute call: python3 ./generate_bin.py

import os

# West command  for generating binary for the platform conexio stratus in non-secure mode
COMMAND = "west build -b conexio_stratus_ns"

# Starting point
# Remove the previous build dir which is same as building with command prestine (-p)
os.system('rm -rf build')

print("------------------------------------------------------------------")
print("              Generating binary for Conexio Stratus               ")
print("This will take a minute or two so go and grab yourself a \u2615 :)")
print("------------------------------------------------------------------")

# Start the new build process
print(COMMAND)
os.system(COMMAND)
