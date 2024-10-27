#!/bin/bash

# Check if exactly one argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <output_directory_prefix>"
    exit 1
fi

# Set the output directory to the input argument plus "jpg"
output_dir_jpg="${1}jpg"
output_dir_png="${1}png"
output_dir_dng="${1}dng"


b_output_dir_jpg="b_${1}jpg"
b_output_dir_png="b_${1}png"
b_output_dir_dng="b_${1}dng"
b_output_dir_dng_f="b_${1}_f_dng"


# Check if any directory already exists
for dir in "$output_dir_jpg" "$output_dir_png" "$output_dir_dng" "$b_output_dir_jpg" "$b_output_dir_png" "$b_output_dir_dng" "$b_output_dir_dng_f"; do
    if [ -d "$dir" ]; then
        echo "Error: Directory '$dir' already exists."
        exit 1
    fi
done


for dir in "$output_dir_jpg" "$output_dir_png" "$output_dir_dng" "$b_output_dir_jpg" "$b_output_dir_png" "$b_output_dir_dng" "$b_output_dir_dng_f"; do
    # Create the directory if it doesn't exist
    mkdir -p "$dir"
    # Empty the directory if it exists
    rm -rf "${dir:?}"/*
done

# Run the Python script with the specified output directory

python main.py -s -t -l "$output_dir_jpg" -f jpg -n 10 -v
python main.py -s -t -l "$output_dir_png" -f png -n 10 -v
python main.py -s -t -l "$output_dir_dng" -f dng -n 10 -v

python main.py -b -s -t -l "$b_output_dir_jpg" -f jpg -n 120 -v
python main.py -b -s -t -l "$b_output_dir_png" -f png -n 20 -v
python main.py -b -s -t -l "$b_output_dir_dng" -f dng -n 120 -v



python main.py -b -s -t -l "$b_output_dir_dng_f" -f dng -n 0 -v
