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
		FOCUSPOS="$POSX"
		ALIGN="$POSY"
		MONPOS="$MONX"
		MONSIZ="$MONW"
		MONOPPOSITEPOS="$MONY"
		MONOPPOSITESIZ="$MONH"
		OP="<"
		;;
	down)
		ATTR="yxi"
		FOCUSPOS="$POSY"
		ALIGN="$POSX"
		MONPOS="$MONY"
		MONSIZ="$MONH"
		MONOPPOSITEPOS="$MONX"
		MONOPPOSITESIZ="$MONW"
		OP=">"
		;;
	up)
		ATTR="yxi"
		FOCUSPOS="$POSY"
		ALIGN="$POSX"
		MONPOS="$MONY"
		MONSIZ="$MONH"
		MONOPPOSITEPOS="$MONX"
		MONOPPOSITESIZ="$MONW"
		OP="<"
		;;
	right)
		ATTR="xyi"
		FOCUSPOS="$POSX"
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
	# $FOCUSPOS is the position of the focused window in the direction we
	# want to focus.  So if we want to focus left/right, $FOCUSPOS will be
	# equal to $POSX, and if we want to focus up/down, $FOCUSPOS will be
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
	awk -v FOCUSPOS="$FOCUSPOS" \
	    -v ALIGN="$ALIGN" \
	    -v MONPOS="$MONPOS" \
	    -v MONSIZ="$MONSIZ" \
	    -v MONOPPOSITEPOS="$MONOPPOSITEPOS" \
	    -v MONOPPOSITESIZ="$MONOPPOSITESIZ" \
	    '
	function abs(n) {
		if (n < 0)
			return -n;
		return n;
	}
	function clientcmp(clia, clib) {
		if (length(clib) == 0)
			return 1
		split(clia, a)
		split(clib, b)
		if (a[2] == ALIGN && b[2] != ALIGN)
			return 1
		if (b[1] '$OP' a[1])
			return 1
		if (a[1] == b[1] && abs(a[2] - ALIGN) < abs(b[2] - ALIGN))
				return 1
		return 0
	}
	BEGIN {
		win=""
	}
	(MONPOS <= $1) &&
	($1 <= MONPOS + MONSIZ) &&
	(MONOPPOSITEPOS <= $2) &&
	($2 <= MONOPPOSITEPOS + MONOPPOSITESIZ) &&
	($1 '$OP' FOCUSPOS) {
		if (clientcmp($0, win))
			win = $0
	}
	END {
		split(win, field)
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
