#!/bin/sh
# find and focus the closest window in a specific direction
# depends on: xrandr, sed, awk, wmctrl, pfw, and lsw

# get current window id
CUR=$(pfw)
POSX=$(wattr x $CUR)
POSY=$(wattr y $CUR)
read MONW MONH MONX MONY << EOF
$(
	xrandr |\
	sed -n '/ connected/{s/.* \([0-9][0-9x+-]*\) .*/\1/; s/[x+-]/ /g; p;}' |\
	awk -v x="$POSX" -v y="$POSY" '
		(x >= $3) && (x <= $3+$1) && (y >= $4) && (y <= $4+$2)
	'
)
EOF

# show usage
usage() {
    echo "usage: $(basename $0) <direction>" >&2
    exit 1
}

# get wid of closest window in given direction
getwid() {
	case $1 in
	left)
		ATTR="xyi"
		POS="$POSX"
		ALIGN="$POSY"
		MONPOS="$MONX"
		MONSIZ="$MONW"
		MONOPPOSITEPOS="$MONY"
		MONOPPOSITESIZ="$MONH"
		OP="<"
		;;
	down)
		ATTR="yxi"
		POS="$POSY"
		ALIGN="$POSX"
		MONPOS="$MONY"
		MONSIZ="$MONH"
		MONOPPOSITEPOS="$MONX"
		MONOPPOSITESIZ="$MONW"
		OP=">"
		;;
	up)
		ATTR="yxi"
		POS="$POSY"
		ALIGN="$POSX"
		MONPOS="$MONY"
		MONSIZ="$MONH"
		MONOPPOSITEPOS="$MONX"
		MONOPPOSITESIZ="$MONW"
		OP="<"
		;;
	right)
		ATTR="xyi"
		POS="$POSX"
		ALIGN="$POSY"
		MONPOS="$MONX"
		MONSIZ="$MONW"
		MONOPPOSITEPOS="$MONY"
		MONOPPOSITESIZ="$MONH"
		OP=">"
		;;
	esac

	# $ATTR is "xyi" or "yxi", depending on the direction we want to
	# focus.  If we are focusing in the x direction (left or right),
	# then our first attribute will be the position in the x direction
	# and our second attribute will be the position in the y direction,
	# and our third attribute will be the window id (thus xyi).  If we
	# are focusing in the y direction, our attributes will be yxi.
	#
	# $POS is the position of the focused window in the direction we
	# want to focus.  So if we want to focus left/right, $POS will be
	# equal to $POSX, and if we want to focus up/down, $POS will be
	# equal to $POSY.
	#
	# $ALIGN is the position other than $POS.
	#
	# $MONPOS and $MONSIZ is the position and size of the monitor the
	# focused window is in the direction we want to focus.
	#
	# $MONOPPOSITEPOS and $MONOPPOSITESIZ is the position and size of
	# the monitor the focused window is in the opposite direction.
	#
	# $OP is either "<" or ">", depending if we want a position lesser
	# than or greater than the position of the focused window.

	lsw |\
	xargs wattr "$ATTR" |\
	awk -v POS="$POS" \
	    -v ALIGN="$ALIGN" \
	    -v MONPOS="$MONPOS" \
	    -v MONSIZ="$MONSIZ" \
	    -v MONOPPOSITEPOS="$MONOPPOSITEPOS" \
	    -v MONOPPOSITESIZ="$MONOPPOSITESIZ" \
	    '
	function pos(s) {
		split(s, field)
		return field[1]
	}
	function sort(array, len, haschanged, tmp, i) {
		haschanged = 1
		while ( haschanged == 1 )
		{
			haschanged = 0
			for (i = 1; i <= len - 1; i++)
			{
				if (pos(array[i]) '$OP' pos(array[i+1]))
				{
					tmp = array[i]
					array[i] = array[i + 1]
					array[i + 1] = tmp
					haschanged = 1
				}
			}
		}
	}
	BEGIN {
		iequal = 1
		idiff = 1
	}
	(MONPOS <= $1) &&
	($1 <= MONPOS + MONSIZ) &&
	(MONOPPOSITEPOS <= $2) &&
	($2 <= MONOPPOSITEPOS + MONOPPOSITESIZ) &&
	($1 '$OP' POS) {
		if ($2 == ALIGN) {
			equal[iequal++] = $0
		} else {
			diff[idiff++] = $0
		}
	}
	END {
		iequal--
		idiff--
		if (iequal > 0) {
			sort(equal, iequal)
			split(equal[1], field)
		} else {
			sort(diff, idiff)
			split(diff[1], field)
		}
		print field[3]
	}
	'
}

# Use the specification of your choice: WASD, HJKL, ←↑↓→, west/north/south/east
case $1 in
    h|a|west|left)  wid=$(getwid left)  ;;
    j|s|south|down) wid=$(getwid down)  ;;
    k|w|north|up)   wid=$(getwid up)    ;;
    l|d|east|right) wid=$(getwid right) ;;
    *)              usage               ;;
esac

test ! -z "$wid" && wmctrl -iR "$wid"
