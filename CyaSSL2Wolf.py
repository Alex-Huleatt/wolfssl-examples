import re
import os
import getopt
import sys
def conv(file,di):
	f = open(file, 'r+')
	st = f.read()
	sorted_keys = sorted(di,key=lambda key:-1*len(key))
	for key in sorted_keys:
		st = st.replace(key, di[key])
	f.seek(0)
	f.write(st)
	f.truncate()
	f.close()

def main():
	replacers = open("ssl.h").read().split("\n")
	di = {}
	for i in replacers:
		splat = i.split(",")
		print(splat[0],splat[1])
		di[splat[0]] = splat[1]
	try:
		opts, args = getopt.getopt(sys.argv[1:], "d:", ["dir="])
	except getopt.GetoptError as err:
		# print help information and exit:
		print(str(err)) # will print something like "option -a not recognized"
		usage()
		sys.exit(2)
	dire = '.'
	for o,a in opts:
		if o == "-d":
			dire = a
		else:
			assert False, "unhandled option"
	print(dire)
	for file in os.listdir('.'):
		if file.endswith(".c"):
			print(file)
			conv(file,di)

if __name__ == "__main__":
	main()




