#!env python

import argparse
import itertools
import subprocess
import struct

# Extract some ctype data from an object and create code to generate it instead.

# Extract symbols info from object file.
# Returns dict between symbol name and 'section' ID.
def extract_symbols(object_file):
  symbol_sects = {}
  dumpbin_out = subprocess.check_output(["dumpbin", "/nologo", "/symbols", object_file], shell=True, universal_newlines=True).split('\n')
  table_lines = itertools.dropwhile(lambda l: l != "COFF SYMBOL TABLE", dumpbin_out)
  next(table_lines)
  table_lines = itertools.takewhile(lambda l: len(l) > 0, table_lines)
  for l in table_lines:
    if l[0] == ' ': continue # Skip 'info' lines
    info, symbol = l.split('|', 1)
    info = info.rstrip()
    symbol = symbol.lstrip()
    info, visibility = info.rsplit(None, 1)
    if visibility != "External": continue # The symbols we're interested in are 'external'
    info_fields = info.split(None, 3)
    if not info_fields[2].startswith('SECT'): continue # Skip undefined data
    symbol_sects[symbol] = info_fields[2][4:]
  return symbol_sects

# Extract raw data for the given object sections
def extract_data(object_file, sections):
  sections_data = {}
  sections = set(sections)
  data_lines = subprocess.check_output(["dumpbin", "/nologo", "/rawdata", object_file], shell=True).decode("cp1252", "replace").split('\n')
  data_lines = map(lambda s: s.rstrip(), data_lines)
  try:
    while True:
      data_lines = itertools.dropwhile(lambda l: not l.startswith("RAW DATA"), data_lines)
      section_id = next(data_lines)[10:] # drop "RAW DATA #"
      if not section_id in sections: continue
      l = next(data_lines)
      section_bytes = bytearray()
      while len(l) > 0:
        hex_data_col = 12 # Column after which the hex data starts
        ascii_dump_col = 12 + 16*3 - 1 # Column after which the ASCII data dump starts
        hex_data = l[hex_data_col:ascii_dump_col]
        section_bytes += bytearray.fromhex(hex_data)
        l = next(data_lines)
      sections_data[section_id] = section_bytes
  except StopIteration:
    pass
  return sections_data

# Delta-encode an array of integers
def delta(values):
  new_values = []
  prev_value = 0
  for v in values:
    new_values.append(v - prev_value)
    prev_value = v
  return new_values

# RLE-encode an array of integers. Returns an array of tuples (count, value)
def rle(values):
  rle_tuples = []
  current_value = None
  count = 0
  for v in values:
    if v != current_value:
      if current_value != None: rle_tuples.append ((current_value, count))
      current_value = v
      count = 1
    else:
      count += 1
  if current_value != None: rle_tuples.append ((current_value, count))
  return rle_tuples

# Print out C++ code to generate an array from RLE data
def print_cxx(cxx_type, cxx_name, orig_size, rle_data):
  def print_tuple_code(prev_value, rle_tuple):
    delta_value, count = rle_tuple
    if count == 1:
      # "Jump" in delta encoding: write actual value
      current_value = prev_value + delta_value
      print("  current = {current_value};".format(current_value=current_value))
      print("  *out_p++ = current;")
    else:
      # Multiple deltas with same value: generate loop
      print("  for (int i = 0; i < {count}; i++) {{ current += {delta_value}; *out_p++ = current; }}".format(count=count, delta_value=delta_value))
      current_value = prev_value + delta_value * count
    return current_value

  print("#pragma section(\".CRT$XIB\")")
  print()
  print("extern \"C\" {cxx_type} {cxx_name}[{orig_size}];".format(cxx_type=cxx_type, cxx_name=cxx_name, orig_size=orig_size))
  print("{cxx_type} {cxx_name}[{orig_size}];".format(cxx_type=cxx_type, cxx_name=cxx_name, orig_size=orig_size))
  print()
  print("static int __cdecl generate_{cxx_name} ()".format(cxx_name=cxx_name))
  print("{")
  print("  {cxx_type} current = 0;".format(cxx_type=cxx_type))
  print("  {cxx_type}* out_p = {cxx_name};".format(cxx_type=cxx_type, cxx_name=cxx_name))
  prev_value = 0
  if rle_data[0][0] == 0:
    # Skip code generation for leading zeroes
    zero_count = rle_data[0][1]
    print("  // Skipping generation of {count} leading zeroes".format(count=zero_count))
    print("  out_p += {count};".format(count=zero_count))
    rle_data = rle_data[1:]
  for rle_tuple in rle_data:
    prev_value = print_tuple_code(prev_value, rle_tuple)
  print("  return 0;")
  print("}")
  print("extern \"C\" __declspec(allocate(\".CRT$XIB\")) int(*init_{cxx_name})() = generate_{cxx_name};".format(cxx_name=cxx_name))
  print()

def strip_trailing_zeroes(values):
  n = 0
  while values[-(n+1)] == 0:
    n += 1
  if n > 0: values = values[:-n]
  return values

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Extract ctype data', fromfile_prefix_chars='@')
  parser.add_argument('input_obj', metavar='OBJ', help='input object')
  args = parser.parse_args()

  symbol_sects = extract_symbols(args.input_obj)
  interesting_symbols = set(['__wctype', '___newctype', '___newclmap', '___newcumap'])
  extracted_data = extract_data(args.input_obj, map(lambda s: symbol_sects[s], interesting_symbols))
  # Data packed as shorts
  for s in ['__wctype', '___newctype']:
    data = list(map(lambda t: t[0], struct.iter_unpack("H", extracted_data[symbol_sects[s]]))) # unpack
    orig_size = len(data)
    data = strip_trailing_zeroes(data)
    print_cxx("unsigned short", s[1:], orig_size, rle(delta(data)))
  # Data packed as bytes (no unpacking necessary)
  for s in ['___newclmap', '___newcumap']:
    data = extracted_data[symbol_sects[s]]
    orig_size = len(data)
    data = strip_trailing_zeroes(data)
    print_cxx("unsigned char", s[1:], orig_size, rle(delta(data)))
  # Declare pointers to tables
  print("extern \"C\" const unsigned short* _pctype = __newctype + 128;")
  print("extern \"C\" const unsigned short* _pwctype = _wctype + 1;")
