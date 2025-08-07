#! /usr/bin/env bash

get_sketch_size() { sed -n 's/Sketch uses \([0-9]\+\) bytes.*/\1/p'; }
get_used_ram() { sed -n 's/Global variables use \([0-9]\+\) .*/\1/p'; }
get_free_ram() { sed -n 's/.* leaving \([0-9]\+\) bytes for local variables.*/\1/p'; }

if [[ $1 = "sketch-size" ]]; then
    size=$(get_sketch_size)
    echo ${size:-n/a}

elif [[ $1 = "used-ram" ]]; then
    size=$(get_used_ram)
    echo ${size:-n/a}

elif [[ $1 = "free-ram" ]]; then
    size=$(get_free_ram)
    echo ${size:-n/a}

elif [[ $1 = "fix-message" ]]; then
    old_subject=$(head -n1 | sed -e 's/^\[[^]]*\] //')
    old_body=$(cat)

    parent=$(git log --format=%P -n1)
    if [[ -n $parent ]]; then
        prev_info=$(git log -n1 "$parent")
        prev_sketch_size=$(get_sketch_size <<< "$prev_info")
        prev_used_ram=$(get_used_ram <<< "$prev_info")
    else
        prev_sketch_size=0
        prev_used_ram=0
    fi

    cur_info=$(make compile-silent 2>&1)
    cur_sketch_size=$(get_sketch_size <<< "$cur_info")
    cur_used_ram=$(get_used_ram <<< "$cur_info")

    delta_sketch_size=$(( cur_sketch_size - prev_sketch_size ))
    delta_used_ram=$(( cur_used_ram - prev_used_ram ))

    if (( delta_sketch_size >= 0 )); then delta_sketch_size=+$delta_sketch_size; fi
    if (( delta_used_ram >= 0 )); then delta_used_ram=+$delta_used_ram; fi

    echo "[$delta_sketch_size/$delta_used_ram] $old_subject"
    echo "$old_body" | sed -e '/Sketch uses /,$d';
    echo "$cur_info"

elif [[ $1 = "amend" ]]; then
    git commit --amend -m "$(git log -n1 --format=%B | $0 fix-message)"

else
    make compile-silent
fi

