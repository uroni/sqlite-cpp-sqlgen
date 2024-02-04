#!/bin/bash

set -e

cp Sample.cpp SampleGen.cpp
cp Sample.h SampleGen.h
sed -i 's/Sample.h/SampleGen.h/g' SampleGen.cpp

cp sample.db samplegen.db
../build/sqlite-cpp-sqlgen samplegen.db SampleGen.cpp