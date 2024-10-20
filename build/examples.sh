#!/bin/sh

DIRS="cat grep shell rule110 game_of_life"

process() {
    awk '
    function embed_file(filename) {
        basename = filename
        gsub(".*/", "", basename)

        if (basename ~ /\.c$/) {
            print "```c"
            print "// " basename
        } else if (basename ~ /\.bs$/) {
            print "```bs"
            print "# " basename
        }

        print ""

        while ((getline line < filename) > 0) {
            print line
        }
        close(filename)
    }

    {
        if ($0 ~ /<!-- embed: (.*) -->/) {
            match($0, /<!-- embed: (.*) -->/, arr)
            file = arr[1]
            embed_file(file)
            print "```"
        } else if ($1 ~ /^#/) {
            sub(/^#/, "##")
            print $0
        } else {
            print $0
        }
    }' $1
}

{
    echo "# Examples"
    for dir in $DIRS; do
        echo ""
        process "examples/$dir/README.md"
    done
} > docs/examples.md

bsdoc docs/examples.md
