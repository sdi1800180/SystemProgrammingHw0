#!/bin/bash

# Check if the script received any arguments
if [ "$#" -eq 0 ]; then
  echo "Usage: $0 <file1> <file2> ... <fileN>"
  exit 1
fi

# Loop through each input file
for file in "$@"; do
  # Check if the file exists and is readable
  if [ ! -r "$file" ]; then
    echo "Error: Cannot read file '$file'"
    continue
  fi

  # Read the file line by line and execute jobs using jobCommander
  while IFS= read -r line; do
    # Skip empty lines and comments (lines starting with #)
    [[ -z "$line" || "${line:0:1}" == "#" ]] && continue

    # Execute the task using jobCommander
    
    ./jobCommander issueJob $line
    # sleep 2
    # echo "DAMIAN"
  done < "$file"
done
