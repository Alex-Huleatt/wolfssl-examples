'''@author AlexHuleatt
This script modifies any of the old CyaSSL files to be updated
It actually changes them, so use with discretion
The ssl.txt file was constructed by modifying the old ssl.h header
'''

import re
import os
import getopt
import sys

RECURSIVE = False
VERBOSE = False
fcount = 0

def conv(fname,di):
	'''Convert a given file using the provided mapping'''
	global fcount
	if VERBOSE:print('>',fname)
	f = open(fname,'r+')
	st = f.read()
	sorted_keys = sorted(di,key=lambda key:-1*len(key))
	for key in sorted_keys:
		st = st.replace(key, di[key])
	f.seek(0)
	f.write(st)
	f.truncate()
	f.close()
	fcount+=1

def main():
	global RECURSIVE, VERBOSE

	#construct the dictionary from the ssl.txt file
	replacers = open("ssl.txt").read().split("\n") 
	di = {}
	for i in replacers:
		splat = i.split(",")
		di[splat[0]] = splat[1]
	try:
		opts, args = getopt.getopt(sys.argv[1:], "hvrd:", [])
	except getopt.GetoptError as err:
		print(str(err))
		usage()
		sys.exit(2)
	dire = '.'
	for o,a in opts:
		if o == "-d":
			dire = a
		elif o == "-r":
			RECURSIVE = True
		elif o=='-v':
			VERBOSE = True
		elif o=='-h':
			print('Usage : Description')
			print('-d directory/to/use : allows the script to start \
			at the given directory, default is the current working directory')
			print('-r : recursively search all subdirectories')
			print('-v : verbose printing enabled')
			print('-h : help')
			return
		else:
			assert False, "unhandled option"

	if VERBOSE: print("---Verbose---")

	if RECURSIVE:
		if VERBOSE:print("---Recursive search---")
		for root, subFolders, files in os.walk(dire):
			for fl in files:
				if (fl.endswith(".c") or fl.endswith(".h") or fl.endswith(".md")):
					conv(os.path.join(root, fl),di)
	else:
		if VERBOSE:print('---Local directory only---')
		for fl in os.listdir(dire):
			if (fl.endswith(".c") or fl.endswith(".h") or fl.endswith(".md")):
				conv(fl,di)


	print("Files potentially modified:",fcount)

if __name__ == "__main__":
	main()