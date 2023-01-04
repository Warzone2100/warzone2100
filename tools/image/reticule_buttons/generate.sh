#!/bin/bash

# This script combines images in ./src/ to create the icons for the reticule buttons.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

source_dir="$DIR/src"
target_dir="$DIR/../../../data/base/images/intfac"
scale=1
hilight_border_thickness=3
hilight_border_outline_thickness=1
blue_border_thickness=3
white_outline_thickness=1
reflection_padding_thickness=2
down_padding=$((blue_border_thickness + white_outline_thickness))
small_width=76
small_height=60
big_width=92
big_height=76

main () {
    for i in build research design intelmap manufacture commanddroid
    do
        generate_reticule_icon $i $((small_width)) $((small_height))
    done

    generate_reticule_icon cancel $((big_width)) $((big_height))

    convert $(border $((small_width + 2 * hilight_border_outline_thickness)) $((small_height + 2 * hilight_border_outline_thickness)) $((hilight_border_thickness))) $target_dir/image_reticule_hilight.png &
    convert $(border $((big_width + 2 * hilight_border_outline_thickness)) $((big_height + 2 * hilight_border_outline_thickness)) $((hilight_border_thickness))) $target_dir/image_cancel_hilight.png &

    convert \
        -gravity center \
        $(shape "$small_width" "$small_height" "#5c5c5cff") \
        $(shape $((small_width - 2 * blue_border_thickness)) $((small_height - 2 * blue_border_thickness)) "#000000ff") \
        -compose over \
        -composite \
        $(shape $((small_width - 2 * down_padding)) $((small_height - 2 * down_padding)) "#5c5c5cff") \
        -compose over \
        -composite \
        $target_dir/image_reticule_grey.png &

    convert \
        $(shape "$small_width" "$small_height" "#cce6ffff") \
        $(shape $((small_width - 2 * blue_border_thickness)) $((small_height - 2 * blue_border_thickness)) "#ffffffff") \
        -gravity center \
        -compose over \
        -composite \
        $target_dir/image_reticule_butdown.png &

    wait
}

rasterized_svg () {
    image="$1"
    width="$2"
    height="$3"
    echo "( -density 1200 -background none $image -resize $((width * scale))x$((height * scale))! )"
}

border () {
    width="$1"
    height="$2"
    thickness="$3"

    echo "(
        $(shape "$width" "$height" "#cce6ffff") \
        (
            $(rasterized_svg "$source_dir/shape.svg" $((width - 2 * thickness)) $((height - 2 * thickness)))
            -channel a -negate +channel
        )
        -gravity center
        -channel A
        -compose multiply
        -composite +channel
    )"
}

shape () {
    width="$1"
    height="$2"
    color="$3"

    echo "(
        ( -size $((width * scale))x$((height * scale)) xc:${color} )
        $(rasterized_svg "$source_dir/shape.svg" "$width" "$height")
        -gravity center
        -channel A
        -compose multiply
        -composite +channel
    )"
}

generate_reticule_icon () {
    icon_name="$1"
    width="$2"
    height="$3"
    glow_padding=$((down_padding))
    reflection_padding=$((down_padding + reflection_padding_thickness))

    reflection_image="
        -compose over
        -composite
        (
            $(rasterized_svg "$source_dir/reflection.svg" $((width - 2 * reflection_padding)) $((height - 2 * reflection_padding)))
            $(rasterized_svg "$source_dir/shape.svg" $((width - 2 * reflection_padding)) $((height - 2 * reflection_padding)))
            -gravity center
            -channel A
            -compose multiply
            -composite +channel
        )
    "

    if [ "$reflection" == '0' ]
    then
        reflection_image=''
    fi

    convert \
        -gravity center \
        $(shape "$width" "$height" "#818eb8ff") \
        $(shape $((width - 2 * blue_border_thickness)) $((height - 2 * blue_border_thickness)) "#000000ff") \
        -compose over \
        -composite \
        \( \
            $(rasterized_svg "$source_dir/$icon_name.svg" "$width" "$height") \
            $(rasterized_svg "$source_dir/shape.svg" $((width - 2 * down_padding)) $((height - 2 * down_padding))) \
            -gravity center \
            -compose copyopacity \
            -composite \
        \) \
        -compose over \
        -layers merge \
        "$target_dir/image_${icon_name}_up.png" &

    convert \
        -gravity center \
        $(shape "$width" "$height" "#cce6ffff") \
        $(shape $((width - 2 * blue_border_thickness)) $((height - 2 * blue_border_thickness)) "#ffffffff") \
        -compose over \
        -composite \
        \( \
            $(rasterized_svg "$source_dir/$icon_name.svg" "$width" "$height") \
            $(rasterized_svg "$source_dir/shape.svg" $((width - 2 * down_padding)) $((height - 2 * down_padding))) \
            -gravity center \
            -compose copyopacity \
            -composite \
        \) \
        -compose over \
        -composite \
        \( \
            \( \
                -modulate 150,300 \
                $(rasterized_svg "$source_dir/$icon_name.svg" "$width" "$height") \
            \) \
            \( \
                \( \
                    $(rasterized_svg "$source_dir/glow_mask.svg" $((width - 2 * glow_padding)) $((height - 2 * glow_padding))) \
                    -alpha set -channel A -function Polynomial 0.7,0 \
                \) \
                $(rasterized_svg "$source_dir/shape.svg" $((width - 2 * glow_padding)) $((height - 2 * glow_padding))) \
                -gravity center \
                -channel A \
                -compose multiply \
                -composite +channel \
            \) \
            -gravity center \
            -compose copyopacity \
            -composite \
        \) \
        $reflection_image \
        -gravity center \
        -compose over \
        -composite \
        "$target_dir/image_${icon_name}_down.png" &
}

main
