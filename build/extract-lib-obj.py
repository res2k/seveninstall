#!env python

# Extract a single object file from a library

import argparse
import os
import shutil
import subprocess

def _extract_lib_member(lib, member, out_name):
  subprocess.check_call(["lib", "/nologo", lib, "/extract:" + member, "/out:" + out_name], shell=True)
  shutil.copystat(lib, out_name)

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Extract a single object file from a library', fromfile_prefix_chars='@')
  parser.add_argument('input_lib', metavar='LIB', help='input library')
  parser.add_argument('obj_file', metavar='OBJ', help='object file to extract')
  parser.add_argument('-o', '--out', dest='out_file', metavar='OUT-OBJ', required=True, help='Output object name')
  args = parser.parse_args()

  library_list = subprocess.check_output(["lib", "/nologo", "/list", args.input_lib], shell=True, universal_newlines=True).split('\n')
  for member in library_list:
    if len(member) == 0: continue
    member_base = os.path.basename(member)
    if member_base == args.obj_file:
      _extract_lib_member (args.input_lib, member, args.out_file)
      break
