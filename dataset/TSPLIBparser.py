# -- coding: utf-8 --
import sys
reload (sys)
sys.setdefaultencoding('utf8')
import math
root_dir = './'
input_file = root_dir + 'ch71009.tsp'
output_file = root_dir + 'ch71009_d.txt'
TNUM = 71010

fin = open(input_file, 'r')
fout = open(output_file, 'w')

coordinate = [[0 for i in xrange(0, 2)] for j in xrange(0, TNUM)]
admatrix = [[0 for i in xrange(0, TNUM)] for j in xrange(0, TNUM)]

for eachline in fin:
	words = eachline.strip('\n').split('\t')
	numid = int(words[0])
	xx = float(words[1])
	yy = float(words[2])
	coordinate[numid][0] = xx
	coordinate[numid][1] = yy

for i in xrange(1, TNUM): 
	for j in xrange(i, TNUM):
		x1 = coordinate[i][0]
		y1 = coordinate[i][1]
		x2 = coordinate[j][0]
		y2 = coordinate[j][1]
		admatrix[i][j] = (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)
		admatrix[j][i] = admatrix[i][j]

for i in xrange(1, TNUM): 
	fout.write(str(admatrix[i][1]))
	for j in xrange(2, TNUM):
		fout.write('\t' + str(admatrix[i][j]))
	fout.write('\n')
