#!/bin/bash

generate_humid=true
if [ "$1" = "-cw" ]; then
  generate_humid=false
  shift
fi

function generate_humid_row() {
  remote_base=kbd_
  name_base=kbd_
  width=60
  space=3
  x=$1; shift
  y=$1; shift;
  while [ "$1" != "" ]; do
     echo "${name_base}$1 KeyboardButton( caption: \"$1\", pos_x: $x, pos_y: $y, remote: \"${remote_base}$1\", width: $width, height: $width);"
	 x=`expr $x + $width + $space`
	shift
  done
}

# keypad_0 KeyButton(value:"0") input;
function generate_cw_keys() {
  name_base=kbd_ # should be the same as humid's remote_base
  while [ "$1" != "" ]; do
     echo "${name_base}$1 KeyButton(value:\"$1\") kbd_input;"
	 shift
  done
}

if $generate_humid; then
  y=100
  height=60
  y_space=3
  generate_humid_row 50 $y 1 2 3 4 5 6 7 8 9 0
  y=`expr $y + $height + $y_space`
  generate_humid_row 50 $y Q W E R T Y U I O P
  y=`expr $y + $height + $y_space`
  generate_humid_row 50 $y A S D F G H J K L : | sed 's/_:/_colon/g'
  y=`expr $y + $height + $y_space`
  generate_humid_row 50 $y Z X C V B N M , . / | sed 's/_,/_comma/g; s/_\./_period/g; s/_\//_slash/g'
else
  generate_cw_keys 1 2 3 4 5 6 7 8 9 0
  generate_cw_keys Q W E R T Y U I O P
  generate_cw_keys A S D F G H J K L : | sed 's/_:/_colon/g'
  generate_cw_keys Z X C V B N M , . / | sed 's/_,/_comma/g; s/_\./_period/g; s/_\//_slash/g'
fi

