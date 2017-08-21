#!env python

import argparse
import os
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser(description='Reassemble a static library')
parser.add_argument('input_lib', metavar='LIB', help='input library')
parser.add_argument('-x', '--exclude', dest='excludes', metavar='OBJ', action='append', help='object files to exclude from library')
parser.add_argument('-X', '--no-include', dest='no_include', metavar='SYMBOL', action='append', help='symbols to remove from \'/include\' directives')
parser.add_argument('-o', '--out', dest='out_file', metavar='OUT-LIB', required=True, help='Output library name')
args = parser.parse_args()

if args.no_include:
  no_include_set = set(args.no_include)
else:
  no_include_set = set()

# Extract .drectve sections from object_file. Returns list of (position, size) tuple
def _extract_drectve(object_file):
  all_drectves = []
  dumpbin_out = subprocess.check_output(["dumpbin", "/nologo", "/section:.drectve", "/rawdata:none", object_file], shell=True, universal_newlines=True).split('\n')
  parse_section = False
  parse_section_field = None
  parsed_drectve_size = None
  parsed_drectve_pos = None
  for line in dumpbin_out:
    if line.startswith(".drectve"):
      parse_section = True
      parse_section_field = 0
    elif parse_section:
      parse_section_field = parse_section_field + 1
      line = line.strip()
      if parse_section_field == 3: # sizeof raw data
        value, descr = line.split(' ', 1)
        parsed_drectve_size = int(value, 16)
      elif parse_section_field == 4: # file pointer to raw data
        value, descr = line.split(' ', 1)
        parsed_drectve_pos = int(value, 16)
      elif len(line) == 0:
        if (parsed_drectve_pos != None) and (parsed_drectve_size != None):
          all_drectves.append((parsed_drectve_pos, parsed_drectve_size))
        parse_section = False
        parse_section_field = None
        parsed_drectve_size = None
        parsed_drectve_pos = None
  return all_drectves

def _filter_drectve(drectve_bytes):
  drectve_string = drectve_bytes.decode()
  drectve_string_lower = drectve_string.lower()
  search_pos = 0
  while search_pos < len(drectve_string):
    include_pos = drectve_string_lower.find("/include", search_pos)
    if include_pos == -1: break
    include_end = drectve_string.find(" ", include_pos)
    if include_end == -1: include_end = len(drectve_string)
    include_arg = drectve_string[include_pos:include_end]
    include_opt, include_sym = include_arg.split(':', 1)
    if include_sym in no_include_set:
      drectve_string = drectve_string[:include_pos] + drectve_string[include_end:]
      drectve_string_lower = drectve_string_lower[:include_pos] + drectve_string_lower[include_end:]
      search_pos = include_pos
    else:
      search_pos = include_end
  return drectve_string.encode().ljust(len(drectve_bytes))

def handle_no_include(object_file):
  any_changes = False
  drectves = _extract_drectve(object_file)
  if len(drectves) == 0: return
  with open(object_file, 'r+b') as obj_file:
    for drectve_pos, drectve_size in drectves:
      obj_file.seek(drectve_pos, os.SEEK_SET)
      drectve_data = obj_file.read(drectve_size)
      filtered_drectve = _filter_drectve(drectve_data)
      if filtered_drectve != drectve_data:
        print(object_file, filtered_drectve)
        assert(len(filtered_drectve) <= len(drectve_data))
        obj_file.seek(drectve_pos, os.SEEK_SET)
        obj_file.write(filtered_drectve)
        any_changes = True
  return any_changes

if args.excludes:
  excluded_set = set(args.excludes)
else:
  excluded_set = set()

library_list = subprocess.check_output(["lib", "/nologo", "/list", args.input_lib], shell=True, universal_newlines=True).split('\n')
temp_dir = tempfile.mkdtemp()
all_members = []
try:
  for member in library_list:
    if len(member) == 0: continue
    member_base = os.path.basename(member)
    if member_base in excluded_set: continue
    out_name = os.path.join(temp_dir, member_base)
    subprocess.check_call(["lib", "/nologo", args.input_lib, "/extract:" + member, "/out:" + out_name], shell=True)
    if not handle_no_include(out_name):
      shutil.copystat(args.input_lib, out_name)
    all_members.append(out_name)
  if os.path.exists(args.out_file):
    os.remove(args.out_file)
  response_file_name = os.path.join(temp_dir, "response.txt")
  with open(response_file_name, "w") as response_file:
    for member in all_members:
      response_file.write(member + '\n')
  try:
    subprocess.check_call(["lib", "/nologo", "/out:" + args.out_file, "@" + response_file_name], shell=True)
  finally:
    os.remove(response_file_name)
finally:
  shutil.rmtree(temp_dir)
