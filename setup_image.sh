#!/usr/bin/env bash

[[ -z $1 ]] && echo "Usage: ./setup_image.sh <path_to_image>" && exit

script_dir="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
image="$1"

side_length=`grep "#define SIDE_LENGTH" "$script_dir/config.h" | awk '{printf $3}'`

convert "$image" -resize $side_length\x$side_length\! "$script_dir/image.png"

python "$script_dir/image_to_binary_data.py" "$script_dir/image.png"

echo "Done. Ensure that you uncommented the IMAGE definition line in config.h"
