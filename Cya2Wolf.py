import re
import os
import getopt
import sys
import mimetypes

RECURSIVE = False
VERBOSE = False
count = 0

def conv(fname,di):
	global count
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
	count+=1

def main():
	global RECURSIVE, VERBOSE
	replacers = open("ssl.txt").read().split("\n")
	di = {}
	for i in replacers:
		splat = i.split(",")
		di[splat[0]] = splat[1]
	try:
		opts, args = getopt.getopt(sys.argv[1:], "hvrd:", [])
	except getopt.GetoptError as err:
		# print help information and exit:
		print(str(err)) # will print something like "option -a not recognized"
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
			print('-d directory/to/use : allows the script to start at the given directory, default is the current working directory')
			print('-r : recursively search all subdirectories')
			print('-v : verbose printing enabled')
			return
		else:
			assert False, "unhandled option"

	if VERBOSE: print("---Verbose---")
	if RECURSIVE:
		if VERBOSE:print("---Recursive search---")
		for root, subFolders, files in os.walk(dire):
			for fl in files:
				if (fl.endswith(".c")):
					conv(os.path.join(root, fl),di)
	else:
		for fl in os.listdir(dire):
			if (fl.endswith(".c")):
				conv(fl,di)


	print("Files potentially modified:",count)

if __name__ == "__main__":
	main()