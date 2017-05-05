import glob
import posixpath
import hashlib
import binascii

def hash_bytestr_iter(bytesiter, hasher, ashexstr=False):
    for block in bytesiter:
        hasher.update(block)
    return (hasher.hexdigest() if ashexstr else hasher.digest())

def file_as_blockiter(afile, blocksize=65536):
    with afile:
        block = afile.read(blocksize)
        while len(block) > 0:
            yield block
            block = afile.read(blocksize)


filelist = []

for filename in glob.iglob('assets/*/**/*.*', recursive=True):
	p = posixpath.join(*filename.split('\\'))
	
	h = hash_bytestr_iter(file_as_blockiter(open(filename, 'rb')), hashlib.md5())
	
	h_str = binascii.hexlify(h).decode()
	
	line = '%s\t%s' % (p, h_str)
	
	print(line)
	filelist.append(line)

with open('assets/list.txt', 'w') as f:
	f.write('\n'.join(filelist))
