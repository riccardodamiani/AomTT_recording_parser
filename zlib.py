
import sys
import zlib
import struct

def compress(filename, eventCount):
    comp = ""
    with open(filename, "rb") as f:
        if(f.closed):
            print("File do not exists: ", filename)
            exit()
        data = f.read()
        try:
            comp = zlib.compress(data)
        except zlib.error as e:
            print(e)
            exit()
    
    with open("compressed", "wb") as f:
        f.write(b"l33t")
        f.write(struct.pack("<I", len(data)))
        f.write(comp)
        ending_sequence = bytearray([0x19] + [0x0]*103 + [0x52, 0x47, 0x19, 0x00, 0xd2, 0x02, 0x96, 0x49])
        filesize = f.tell()
        f.write((ending_sequence))
        f.write(struct.pack("<I", filesize))
        #eventCount = 0x3102
        f.write(struct.pack("<I", eventCount))

def decompress(filename):
    with open(filename, "rb") as f:
        magic = f.read(4)
        if magic != b"l33t":
            print("Bad magic value")
            exit()
        size = struct.unpack("<I", f.read(4))[0]
        body = f.read()
        try:
            decomp = zlib.decompress(body)
        except zlib.error as e:
            print(e)
            print("Recording corrupted")
            exit()
        if size != len(decomp):
            print("Error in decompression. File might be corrupted.")
    
    with open("decompressed", "wb") as f:
        f.write(decomp)
        
def main():
    if(len(sys.argv) < 3):
        printUsage()
    if(sys.argv[1] == '-c'):
        if(len(sys.argv) < 4):
            printUsage()
        compress(sys.argv[3], int(sys.argv[2]))
    elif(sys.argv[1] == '-d'):
        decompress(sys.argv[2])
    else:
        printUsage()

def printUsage():
    print("Usage: [flag] [EventCount] [filename]"
            "\n [flag]"
            "   \n\t-c = compress"
            "       \n\t\t[EventCount] = number of game events. Required."
            "   \n\t-d = decompress\n")
    exit()

if __name__ == '__main__':
    main()
