#!/bin/bash

# This script combines images in data/base/images/src/ to create the icons for the reticule buttons.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

source_dir="$DIR/src"
target_dir="$DIR/../../../data/base/images/intfac"
scale=2
hilight_border_thickness=1
blue_border_thickness=1
white_outline_thickness=1
small_width=38
small_height=30
big_width=46
big_height=38

main () {
    for i in build research design intelmap manufacture commanddroid
    do
        generate_reticule_icon $i $((small_width)) $((small_height))
    done

    generate_reticule_icon cancel $((big_width)) $((big_height))

    convert $(border $((small_width + 2 * hilight_border_thickness)) $((small_height + 2 * hilight_border_thickness)) $((hilight_border_thickness + 1))) $target_dir/image_reticule_hilight.png &
    convert $(border $((big_width + 2 * hilight_border_thickness)) $((big_height + 2 * hilight_border_thickness)) $((hilight_border_thickness + 1))) $target_dir/image_cancel_hilight.png &

    convert \
        $(rasterized_svg "$source_dir/shape.svg" $small_width $small_height) \
        \( \
            $(rasterized_svg "$source_dir/shape.svg" $((small_width - 2 * blue_border_thickness)) $((small_height - 2 * blue_border_thickness))) \
            -modulate 50,0 \
        \) \
        -gravity center \
        -compose over \
        -composite \
        $target_dir/image_reticule_grey.png &

    convert \
        $(rasterized_svg "$source_dir/shape.svg" $((small_width)) $((small_height))) \
        \( \
            $(rasterized_svg "$source_dir/shape.svg" $((small_width - 2 * blue_border_thickness)) $((small_height - 2 * blue_border_thickness))) \
            -modulate 200 \
        \) \
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
        $(rasterized_svg "$source_dir/shape.svg" "$width" "$height")
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

generate_reticule_icon () {
    icon_name="$1"
    width="$2"
    height="$3"

    convert \
        $(rasterized_svg "$source_dir/shape.svg" "$width" "$height") \
        \( \
            $(rasterized_svg "$source_dir/$icon_name.svg" "$width" "$height") \
            $(rasterized_svg "$source_dir/shape.svg" $((width - 2 * blue_border_thickness)) $((height - 2 * blue_border_thickness))) \
            -gravity center \
            -compose copyopacity \
            -composite \
        \) \
        -compose over \
        -layers merge \
        "$target_dir/image_${icon_name}_up.png" &

    down_padding=$((blue_border_thickness + white_outline_thickness))
    glow_padding=$((down_padding))
    reflection_padding=$((down_padding + 1))
    convert \
        -gravity center \
        $(rasterized_svg "$source_dir/shape.svg" "$width" "$height") \
        \( \
            -modulate 300,100 \
            $(rasterized_svg "$source_dir/shape.svg" $((width - 2 * blue_border_thickness)) $((height - 2 * blue_border_thickness))) \
        \) \
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
                $(rasterized_svg "$source_dir/glow_mask.svg" $((width - 2 * glow_padding)) $((height - 2 * glow_padding))) \
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
        -compose over \
        -composite \
        \( \
            $(rasterized_svg "$source_dir/reflection.svg" $((width - 2 * reflection_padding)) $((height - 2 * reflection_padding))) \
            $(rasterized_svg "$source_dir/shape.svg" $((width - 2 * reflection_padding)) $((height - 2 * reflection_padding))) \
            -gravity center \
            -channel A \
            -compose multiply \
            -composite +channel \
        \) \
        -gravity center \
        -compose over \
        -composite \
        "$target_dir/image_${icon_name}_down.png" &
}

main
