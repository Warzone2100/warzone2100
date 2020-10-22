#!/bin/bash

# This script combines images in src/ to create the icons for the reticule buttons.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

source_dir="$DIR/src"
target_dir="$DIR/intfac"
border_thickness=2
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

    convert $(border $((small_width + 4)) $((small_height + 4)) 3) $target_dir/image_reticule_hilight.png &
    convert $(border $((big_width + 4)) $((big_height + 4)) 3) $target_dir/image_cancel_hilight.png &

    convert \
        $(rasterized_svg "$source_dir/shape.svg" $((small_width)) $((small_height))) \
        \( \
            $(rasterized_svg "$source_dir/shape.svg" $((small_width - 4)) $((small_height - 4))) \
            -modulate 50,0 \
        \) \
        -gravity center \
        -compose over \
        -composite \
        $target_dir/image_reticule_grey.png &

    convert \
        $(rasterized_svg "$source_dir/shape.svg" $((small_width)) $((small_height))) \
        \( \
            $(rasterized_svg "$source_dir/shape.svg" $((small_width - 4)) $((small_height - 4))) \
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
    echo "( -density 1200 -background none $image -resize ${width}x${height}! )"
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
            $(rasterized_svg "$source_dir/shape.svg" $((width - border_thickness * 2)) $((height - border_thickness * 2))) \
            -gravity center \
            -compose copyopacity \
            -composite \
        \) \
        -compose over \
        -layers merge \
        "$target_dir/image_${icon_name}_up.png" &

    convert \
        $(rasterized_svg "$source_dir/shape.svg" "$width" "$height") \
        \( \
            $(rasterized_svg "$source_dir/$icon_name.svg" "$width" "$height") \
            $(rasterized_svg "$source_dir/shape.svg" $((width - border_thickness * 2)) $((height - border_thickness * 2))) \
            -gravity center \
            -compose copyopacity \
            -composite \
        \) \
        \( \
            \( \
                -modulate 150,300 \
                $(rasterized_svg "$source_dir/$icon_name.svg" "$width" "$height") \
            \) \
            \( \
                $(rasterized_svg "$source_dir/glow_mask.svg" $((width - 6)) $((height - 6))) \
                $(rasterized_svg "$source_dir/shape.svg" $((width - 6)) $((height - 6))) \
                -gravity center \
                -channel A \
                -compose multiply \
                -composite +channel \
            \) \
            -compose copyopacity \
            -composite \
        \) \
        -compose over \
        +flatten \
        "$target_dir/image_${icon_name}_down.png" &
}

main
