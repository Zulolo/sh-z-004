import zlib
import re
import os

buffersize = 65536

fw_version = 0
slave_id = 2
fw_length = 0
magic_number = 0x5AA55A4B

def int_to_bytes(value, length):
    result = []
    for i in range(0, length):
        result.append(value >> (i * 8) & 0xff)
#    result.reverse()
    return result

print(os.getcwd())

# get fw version from header file
# with open('G:\Work\io\design\code\io_2eth_stm32f4\Inc\sh_z_002.h', 'rb') as afile:
with open('..\Inc\sh_z_002.h', 'rb') as afile:
    lines = afile.readlines()
    for line in lines:
        if line.startswith('#define SH_Z_002_VERSION'):
            print(line)
            ss = re.findall(r'0x[0-9A-F]+', line, re.I)
            fw_version = int(ss[0], 0)

# get fw length
# fw_length = os.stat('G:\Work\io\design\code\io_2eth_stm32f4\MDK-ARM\io_2eth_stm32f4\io_2eth_stm32f4.bin').st_size
fw_length = os.stat('.\io_2eth_stm32f4\io_2eth_stm32f4.bin').st_size
fw_length /= 4;
print(fw_length)

# get CRC
# with open('G:\Work\io\design\code\io_2eth_stm32f4\MDK-ARM\io_2eth_stm32f4\io_2eth_stm32f4.bin', 'rb') as afile:
crcvalue = 0xFFFFFFFF
crcvalue = zlib.crc32(buffer(bytearray(int_to_bytes(slave_id, 4))), crcvalue)
crcvalue = zlib.crc32(buffer(bytearray(int_to_bytes(fw_version, 4))), crcvalue)
crcvalue = zlib.crc32(buffer(bytearray(int_to_bytes(fw_length, 4))), crcvalue)
with open('.\io_2eth_stm32f4\io_2eth_stm32f4.bin', 'rb') as afile:
    buffr = afile.read(buffersize)   
    while len(buffr) > 0:
        crcvalue = zlib.crc32(buffr, crcvalue)
        buffr = afile.read(buffersize)

print(format(crcvalue & 0xFFFFFFFF, '08x'))

# new_file_name = 'G:\Work\io\design\code\io_2eth_stm32f4\MDK-ARM\io_2eth_stm32f4\\fw_002_v' + format(fw_version, 'x') + '.bin'
new_file_name = '.\io_2eth_stm32f4\sh_z_002_update.bin' 
print(new_file_name)
with open(new_file_name, 'w+b') as bfile:
    bfile.write(bytearray(int_to_bytes(magic_number, 4)))
    bfile.write(bytearray(int_to_bytes(crcvalue, 4)))
    bfile.write(bytearray(int_to_bytes(slave_id, 4)))
    bfile.write(bytearray(int_to_bytes(fw_version, 4)))
    bfile.write(bytearray(int_to_bytes(fw_length, 4)))
#    with open('G:\Work\io\design\code\io_2eth_stm32f4\MDK-ARM\io_2eth_stm32f4\io_2eth_stm32f4.bin', 'rb') as cfile:
    with open('.\io_2eth_stm32f4\io_2eth_stm32f4.bin', 'rb') as cfile:
        buffr = cfile.read(buffersize)   
        while len(buffr) > 0:
            bfile.write(buffr)
            buffr = cfile.read(buffersize)    
        
    
