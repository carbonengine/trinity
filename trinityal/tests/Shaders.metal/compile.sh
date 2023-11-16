#!/bin/zsh
# NOTE: Remove "-gline-tables-only -MO" flags if you don't want shader source debugging.
# setopt NULL_GLOB
rm *.bin
for file in *.psh; do
	echo $file
	xcrun metal -x metal -gline-tables-only -MO $file -o ${file:r}.ps.bin
done
for file in *.vsh; do
	echo $file
	xcrun metal -x metal -gline-tables-only -MO $file -o ${file:r}.vs.bin
done
for file in *.csh; do
	echo $file
	xcrun metal -x metal -gline-tables-only -MO $file -o ${file:r}.cs.bin
done
for file in *.bin; do
	python ../bin2h.cmd $file ${file:r}.h
done
rm *.bin
# unsetopt NULL_GLOB
