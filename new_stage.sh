#!/bin/bash

DESTINATION="$1"
SOURCE="$2"

if [ -d $DESTINATION ]; then
    echo "Destination path already exists"
    exit 1
fi 

if [ ! -d $SOURCE ]; then 
    echo "Source path doesn't exist"
    exit 1
fi 

echo "SOURCE:   $SOURCE"
echo "DESTINATION:  $DESTINATION"

cp -r "$SOURCE"/* "$DESTINATION"

