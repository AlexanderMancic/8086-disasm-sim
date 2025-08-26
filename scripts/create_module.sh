#!/usr/bin/bash

NAME=$1

if [[ -z "$NAME" ]]; then
	echo "Usage: $0 <module_name>"
	exit 1
fi

CPP_FILE="src/$NAME.cpp"
H_FILE="include/$NAME.h"

if [[ -f $CPP_FILE || -f $H_FILE ]]; then
	echo "Files already exist!"
	exit 1
fi

echo "#include \"$NAME.h\"" > $CPP_FILE
touch $H_FILE

echo "Created $CPP_FILE and $H_FILE"
