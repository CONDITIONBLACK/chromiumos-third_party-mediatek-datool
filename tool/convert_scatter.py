#!/usr/bin/python

import struct
import yaml
import sys

f = open(sys.argv[1], "r");
c = f.read();
data=yaml.load(c)
f.close()

g = open(sys.argv[2], "wb");
print("Found {} entries".format(len(data)))
for datum in data:
    if datum['is_download']:
        print("{}: {}: {}: 0x{:x}(0x{:x}): 0x{:x}; {}".format(
            datum['partition_index'],
            datum['partition_name'],
            datum['region'],
            datum['linear_start_addr'],
            datum['physical_start_addr'],
            datum['partition_size'],
            datum['type']
            )
            )
    name=bytearray(16)
    for j, n in enumerate(datum['partition_name']):
        name[j] = ord(n)
    g.write(name)
    g.write(struct.pack("<I", datum['is_download']))
    g.write(struct.pack("<I", datum['d_type']))
    g.write(struct.pack("<I", datum['is_reserved']))
    g.write(struct.pack("<I", datum['boundary_check']))
    g.write(struct.pack("<Q", datum['partition_size']))
    g.write(struct.pack("<Q", datum['linear_start_addr']))
    g.write(struct.pack("<Q", datum['physical_start_addr']))
    g.write(struct.pack("<I", datum['reserve']))
    partition_index=bytearray(16)
    for j, n in enumerate(datum['partition_index']):
        partition_index[j] = ord(n)
    g.write(partition_index)
    region=bytearray(16)
    for j, n in enumerate(datum['region']):
        region[j] = ord(n)
    g.write(region)
    storage=bytearray(16)
    for j, n in enumerate(datum['storage']):
        storage[j] = ord(n)
    g.write(storage)
    type=bytearray(16)
    for j, n in enumerate(datum['type']):
        type[j] = ord(n)
    g.write(type)
    operation=bytearray(16)
    for j, n in enumerate(datum['operation_type']):
        operation[j] = ord(n)
    g.write(operation)
    file=bytearray(100)
    for j, n in enumerate(datum['file_name']):
        file[j] = ord(n)
    g.write(file)
g.close()
