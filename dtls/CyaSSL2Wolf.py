import re
import os

def conv(file):
	global di
	f = open(file, 'r+')
	st = f.read()
	sorted_keys = sorted(di,key=lambda key:-1*len(key))
	for key in sorted_keys:
		st = st.replace(key, di[key])
	f.seek(0)
	f.write(st)
	f.truncate()
	f.close()

replacers = open("ssl.h").read().split("\n")
di = {}
for i in replacers:
	splat = i.split(",")
	print(splat[0],splat[1])
	di[splat[0]] = splat[1]

#ditry = input("Directory:")
for file in os.listdir('.'):
	if file.endswith(".c"):
		print(file)
		conv(file)







