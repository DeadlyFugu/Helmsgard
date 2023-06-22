import struct
import sys

def align16(x):
    while x % 16 != 0:
        x += 1
    return x

def readFileText(path):
    with open(path, 'r') as f:
        return f.read()

def readFileBin(path):
    with open(path, 'rb') as f:
        return f.read()

class ArcBuilder:
    def __init__(self):
        self.entries = []
    def addFile(self, name, content):
        if len(name) > 24:
            print(f"warning: '{name}' will be truncated")
        self.entries.append({ "name": name, "content": content })
        print(f"added {name} ({len(content)} bytes)")

    def write(self, outpath):
        offset = 16 + 32 * len(self.entries)
        for entry in self.entries:
            entry["offset"] = offset
            offset += align16(len(entry["content"]))
        with open(outpath, 'wb') as f:
            f.write(struct.pack('<4sii4x', b'ARC0', 100, len(self.entries)))
            for entry in self.entries:
                f.write(struct.pack('<ii24s', entry["offset"], len(entry["content"]), entry["name"].encode()))
            for entry in self.entries:
                f.seek(entry["offset"])
                f.write(entry["content"])

def makeArcFromFiles(outfile, infiles):
    # ftable = []
    # offset = align16(4 + (64+8) * len(infiles))
    # for fpath in infiles:
    #     with open(fpath, 'rb') as f:
    #         filedata = f.read()
    #     ftable.append((fpath.split('/')[-1], offset, filedata))
    #     offset = align16(offset + len(filedata))
    # arcdata = struct.pack('<i', len(infiles))
    # for entry in ftable:
    #     arcdata += struct.pack('<64sii', entry[0].encode(), entry[1], len(entry[2]))
    # for entry in ftable:
    #     arcdata += entry[2]
    # with open(outfile, 'wb') as f:
    #     f.write(arcdata)

    #zdata = zlib.compress(arcdata)
    #with open(outfile, 'wb') as f:
    #    f.write(struct.pack('<4siii', b'ARC0', 200, len(arcdata), len(zdata)))
    #    f.write(zdata)

    # alloc file offsets
    offset = 16 + 32 * len(infiles)
    ftable = []
    for path in infiles:
        with open(path, 'rb') as f:
            data = f.read()
        ftable.append((path.split('/')[-1], offset, data))
        offset += align16(len(data))
    with open(outfile, 'wb') as f:
        f.write(struct.pack('<4sii4x', b'ARC0', 200, len(infiles)))
        for entry in ftable:
            f.write(struct.pack('<ii24s', entry[1], len(entry[2]), entry[0].encode()))
        for entry in ftable:
            f.seek(entry[1])
            f.write(entry[2])

if __name__ == "__main__":
    if len(sys.argv) == 1:
        print("usage: arctool.py <outfile.arc> [infiles...]")
    else:
        # def bunpath(f):
        #     return os.path.join(sys.argv[1], f)
        # with open(bunpath('_bundle.yml'), 'r') as f:
        #     bundleyml = yaml.safe_load(f)
        # #makeArcFromFiles(sys.argv[1] + '.bun', )
        # arc = ArcBuilder()
        # for k, v in bundleyml["rawfiles"].items():
        #     arc.addFile('/'+k, readFileBin(bunpath(v)))
        # for k, v in bundleyml["modules"].items():
        #     arc.addFile('M'+k, readFileText(bunpath(v)).encode())
        # arc.write(sys.argv[1] + '.bun')
        arc = ArcBuilder()
        for path in sys.argv[2:]:
            try:
                arc.addFile(path, readFileBin(path))
            except Exception as e:
                print(e)
        arc.write(sys.argv[1])
