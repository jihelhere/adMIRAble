#!/usr/bin/env python2
import sys

if len(sys.argv) != 2:
    sys.stderr.write("USAGE: %s <num-labels>\n" % sys.argv[0])
    sys.stderr.write("Converts multiclass examples from the svm_light/mlcomp format to the reranker format\n")
    sys.exit(1)

num_labels = int(sys.argv[1])
for line in sys.stdin:
    tokens = line.strip().split()
    for label in range(1, num_labels + 1):
        output = []
        if tokens[0] == str(label):
            output.append("1")
            output.append("nbe:0")
        else:
            output.append("0")
            output.append("nbe:1")
        for token in tokens[1:]:
            output.append("%d#%s" % (label, token))
        print " ".join(output)
    print


