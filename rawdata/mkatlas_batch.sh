#!/bin/bash
# Run this from CMR's root directory

set -e

SRCDIR=rawdata/sprites
OUTDIR=Data/Sprites
MKATLAS="python3 $SRCDIR/../mkatlas.py"

$MKATLAS $SRCDIR/trackselectmp  -o ${OUTDIR}/trackselectmp  -a 512x4096 --padding 0
$MKATLAS $SRCDIR/trackselectsp  -o ${OUTDIR}/trackselectsp  -a 512x4096 --padding 0
$MKATLAS $SRCDIR/effects        -o ${OUTDIR}/effects        -a 512x512 --no-crop
$MKATLAS $SRCDIR/infobar        -o ${OUTDIR}/infobar        -a 512x512
$MKATLAS $SRCDIR/menus          -o ${OUTDIR}/menus          -a 128x512
$MKATLAS $SRCDIR/rockfont       -o ${OUTDIR}/rockfont       -a 512x512 --font
$MKATLAS $SRCDIR/wallfont       -o ${OUTDIR}/wallfont       -a 512x512 --font
$MKATLAS $SRCDIR/scoreboard     -o ${OUTDIR}/scoreboard     -a 1024x1024
